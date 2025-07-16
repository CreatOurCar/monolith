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

  while (TRUE) {
    do {
      if ((ret = xQueueReceive(logqueue, &log, 0)) == pdTRUE) {
        write(fd, &log, sizeof(log));
      }
    } while (ret == pdTRUE);

    if (ret == pdTRUE) {
      if (fsync(fd) != 0 && !IS_FATAL(SD)) {
        FATAL_LOG(SD, "fsync failure");
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
    FATAL_LOG(SD, "mount failure");
  }

  // set log file
  char logpath[64];
  struct tm tp;
  struct tm *tm = gmtime_r(&boot.tv_sec, &tp);
  strftime(logpath, sizeof(logpath), "/sdcard/%Y-%m-%d-%H-%M-%S.log", tm);

  int fd = open(logpath, O_RDWR | O_CREAT | O_TRUNC, 0);

  if (fd < 0) {
    FATAL_LOG(SD, "file open failure");
  }

  // create log queue and sdcard task
  logqueue = xQueueCreate(32, sizeof(log_t));

  if (xTaskCreatePinnedToCore(task_sdcard, "sdcard", 4096, (void *)fd, 7, NULL, 0) != pdPASS) {
    FATAL_LOG(SD, "task create failure");
  }

  INFO(SD, "log file: %s", logpath);
}
