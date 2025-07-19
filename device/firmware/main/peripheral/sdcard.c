#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "main.h"

#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"

extern struct timeval boot;

/*******************************************************************************
 * save log queue to SD card every 1000 ms
 ******************************************************************************/
static void task_sdcard(void *pvParameters) {
  int fd = (int)pvParameters;
  int ret;
  log_t log;
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (true) {
    do {
      if ((ret = xQueueReceive(logqueue, &log, 0)) == true) {
        write(fd, &log, sizeof(log));
      }
    } while (ret == true);

    if (ret == true) {
      if (fsync(fd) != 0 && !IS_FATAL(&logbuf.run, SD)) {
        FATAL_LOG(&logbuf.run, SD, "fsync failure");
      }

      INFO(SD, "log sync complete");
    }

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
  }
}

/*******************************************************************************
 * init SDIO, mount filesystem and create task
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
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  sdmmc_slot_config_t slot_config = {
    .clk   = GPIO_NUM_39,
    .cmd   = GPIO_NUM_40,
    .d0    = GPIO_NUM_38,
    .d1    = GPIO_NUM_37,
    .d2    = GPIO_NUM_42,
    .d3    = GPIO_NUM_41,
    .cd    = SDMMC_SLOT_NO_CD,
    .wp    = SDMMC_SLOT_NO_WP,
    .width = 4,
    .flags = 0,
  };

  if (esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card) != ESP_OK) {
    FATAL_LOG(&init, SD, "mount failure");
    goto finish;
  }

  // set log file
  char logpath[64];
  struct tm tp;
  struct tm *tm = gmtime_r(&boot.tv_sec, &tp);
  strftime(logpath, sizeof(logpath), "/sdcard/%Y-%m-%d-%H-%M-%S.log", tm);

  int fd = open(logpath, O_RDWR | O_CREAT | O_TRUNC, 0);

  if (fd < 0) {
    FATAL_LOG(&init, SD, "file open failure");
  }

  // create log queue and sdcard task
  logqueue = xQueueCreate(32, sizeof(log_t));

  if (xTaskCreatePinnedToCore(task_sdcard, "sdcard", 4096, (void *)fd, 7, NULL, CORE0) != pdPASS) {
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
