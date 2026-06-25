#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "main.h"

#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "driver/twai.h"
#include "esp_vfs_fat.h"

#define SD_SPI_HOST  SPI2_HOST
#define SD_PIN_SCK   GPIO_NUM_39
#define SD_PIN_MOSI  GPIO_NUM_40
#define SD_PIN_MISO  GPIO_NUM_48
#define SD_PIN_CS    GPIO_NUM_41

extern struct timeval boot;

char logpath[64];

/*******************************************************************************
 * save log queue to SD card every 1000 ms
 ******************************************************************************/
static void task_sdcard(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();

  int ret;
  int fd = (int)pvParameters;
  log_t log;
  int write_count = 0;
  int cycle_count = 0;

  while (true) {
    if (file_op_busy) {
      if (write_count > 0) {
        fsync(fd);
        write_count = 0;
        cycle_count = 0;
      }
      xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
      continue;
    }

    do {
      if ((ret = xQueueReceive(logqueue, &log, 0)) == pdTRUE) {
        write(fd, &log, sizeof(log));
        write_count++;
      }
    } while (ret);

    cycle_count++;

    if (write_count > 0 && (write_count >= 512 || cycle_count >= 3)) {
      if (fsync(fd) != 0 && !IS_FATAL(&logbuf.run, SD)) {
        FATAL_LOG(&logbuf.run, SD, "fsync failure");
      }
      write_count = 0;
      cycle_count = 0;
      INFO(SD, "log sync");
    }

    xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
  }
}

/*******************************************************************************
 * init SDSPI, mount filesystem and create task
 ******************************************************************************/
void sdcard_init(void) {
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed   = false,
    .max_files                = 4,
    .allocation_unit_size     = 32 * 512,
    .disk_status_check_enable = false,
    .use_one_fat              = false,
  };

  sdmmc_card_t *card;
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot         = SD_SPI_HOST;
  host.max_freq_khz = SDMMC_FREQ_PROBING;  // 400 kHz — 점퍼선 SPI 모듈은 20MHz 기본값에서 읽기 손상

  spi_bus_config_t bus_cfg = {
    .mosi_io_num     = SD_PIN_MOSI,
    .miso_io_num     = SD_PIN_MISO,
    .sclk_io_num     = SD_PIN_SCK,
    .quadwp_io_num   = -1,
    .quadhd_io_num   = -1,
    .max_transfer_sz = 4000,
  };

  if (spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO) != ESP_OK) {
    FATAL_LOG(&init, SD, "SPI bus init failure");
    goto finish;
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = SD_PIN_CS;
  slot_config.host_id = SD_SPI_HOST;

  if (esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card) != ESP_OK) {
    FATAL_LOG(&init, SD, "mount failure");
    goto finish;
  }

  // set log file
  setenv("TZ", storage.device.tz, 1);
  tzset();

  struct tm tp;
  struct tm *tm = localtime_r(&boot.tv_sec, &tp);

  strftime(logpath, sizeof(logpath), "/sdcard/%Y-%m-%d-%H-%M-%S.log", tm);

  setenv("TZ", "UTC", 1);
  tzset();

  int fd = open(logpath, O_RDWR | O_CREAT | O_TRUNC, 0);

  if (fd < 0) {
    FATAL_LOG(&init, SD, "file open failure");
  }

  // create log queue and sdcard task
  logqueue    = xQueueCreate(2560, sizeof(log_t));
  syslogqueue = xQueueCreate(32, sizeof(log_t));
  canlogqueue = xQueueCreate(1024, sizeof(log_t));
  cantxqueue  = xQueueCreate(4, sizeof(twai_message_t));

  if (xTaskCreate(task_sdcard, "sdcard", 4096, (void *)fd, 7, NULL) != pdPASS) {
    FATAL_LOG(&init, SD, "task create failure");
    goto finish;
  }

  INFO(SD, "log file: %s", logpath);

  log_t boot_record;
  boot_record.payload.boot.protocol_version = PROTOCOL_VERSION;
  boot_record.payload.boot.boot_time        = (uint64_t)boot.tv_sec;
  memcpy(boot_record.payload.boot.mac, storage.wifi.mac, sizeof(storage.wifi.mac));

  if (LOG(LOG_TYPE_BOOT, &boot_record) != true) {
    FATAL_LOG(&init, SD, "boot record failure");
    goto finish;
  }

finish:
  if (IS_OK(&init, SD)) {
    CLEAR_ALL(&logbuf.run, SD);
  } else {
    COPY_STATE(&logbuf.run, &init, SD);
  }
}
