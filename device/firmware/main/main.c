#include "main.h"

TaskHandle_t ledtask   = NULL;
QueueHandle_t ledqueue = NULL;
QueueHandle_t logqueue = NULL;

static void rtc_init(void);
static void sdcard_init(struct timeval *tv);
static void peripheral_task_init(void);

static void reset_isr(void *arg);
static void task_led(void *pvParameters);
static void task_sdcard(void *pvParameters);

/*******************************************************************************
 * core system initialization entry point
 ******************************************************************************/
void app_main(void) {
  gpio_config_t gpio;

  /*** LED ***/
  gpio.pin_bit_mask = (1ULL << GPIO_NUM_5);
  gpio.mode         = GPIO_MODE_OUTPUT_OD;
  gpio.intr_type    = GPIO_INTR_DISABLE;
  gpio.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;

  ledqueue = xQueueCreate(4, sizeof(state_t));

  if (gpio_config(&gpio) != ESP_OK || xTaskCreatePinnedToCore(task_led, "led", 1024, NULL, 5, &ledtask, 0) != pdPASS) {
    ESP_LOGW("LED", "config failure");
  }

  /*** RST ***/
  gpio.pin_bit_mask = (1ULL << GPIO_NUM_21);
  gpio.mode         = GPIO_MODE_INPUT;
  gpio.intr_type    = GPIO_INTR_ANYEDGE;
  gpio.pull_down_en = GPIO_PULLDOWN_ENABLE;

  if (gpio_config(&gpio) != ESP_OK || gpio_install_isr_service(0) != ESP_OK ||
      gpio_isr_handler_add(GPIO_NUM_21, reset_isr, NULL) != ESP_OK) {
    ESP_LOGW("RST", "config failure");
  }

  /*** NVS ***/
  if (nvs_flash_init() != ESP_OK) {
    STATE_SYSLOG(STATE_FATAL, "NVS", "flash init failure", "NVS_INIT_FAIL");
  }

  /*** RTC ***/
  rtc_init();

  // read boot time
  struct timeval tv;
  gettimeofday(&tv, NULL);

  /*** SDIO ***/
  sdcard_init(&tv);

  // save boot record
  log_t boot_record;
  boot_record.payload.boot.protocol_version = PROTOCOL_VERSION;
  boot_record.payload.boot.boot_time        = (uint64_t)tv.tv_sec;
  esp_read_mac(boot_record.payload.boot.mac, ESP_MAC_WIFI_STA);

  if (LOG(LOG_TYPE_BOOT, &boot_record) != pdTRUE) {
    STATE_ESPLOG(STATE_FATAL, "CORE", "boot record failure");
  }

  /*** Wi-Fi ***/
  if (xTaskCreatePinnedToCore(task_network, "network", 4096, NULL, 5, NULL, 0) != pdPASS) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "task creation failure", "NET_NEWTASK_FAIL");
  }

  peripheral_task_init();
}

/*******************************************************************************
 * rtc_init(): init RTC and set system time
 ******************************************************************************/
static void rtc_init(void) {
  i2c_master_bus_handle_t i2c0_handle;
  i2c_master_bus_config_t i2c_mst_config = {
    .clk_source                   = I2C_CLK_SRC_DEFAULT,
    .i2c_port                     = I2C_NUM_0,
    .scl_io_num                   = GPIO_NUM_10,
    .sda_io_num                   = GPIO_NUM_9,
    .glitch_ignore_cnt            = 7,
    .flags.enable_internal_pullup = false,
  };

  if (i2c_new_master_bus(&i2c_mst_config, &i2c0_handle) != ESP_OK) {
    STATE_ESPLOG(STATE_ERR, "RTC", "I2C0 init failure");
  }

  i2c_master_dev_handle_t rtc_handle;
  i2c_device_config_t rtc_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x51,
    .scl_speed_hz    = 100000,
  };

  if (i2c_master_bus_add_device(i2c0_handle, &rtc_cfg, &rtc_handle) != ESP_OK) {
    STATE_ESPLOG(STATE_ERR, "RTC", "device init failure");
  }

  uint8_t tx[1] = { 0x02 };  // VL_seconds register address
  uint8_t rx[7] = { 0 };     // 0x02 VL_seconds to 0x08 Years register

  if (i2c_master_transmit_receive(rtc_handle, tx, sizeof(tx), rx, sizeof(rx), 100) != ESP_OK) {
    memset(rx, 0, sizeof(rx));
    STATE_ESPLOG(STATE_ERR, "RTC", "read time transfer failure");
  }

  // rtc has valid time set
  if (rx[6] != 0x00) {
    struct tm tm = {
      .tm_sec  = BCD_TO_DEC(rx[0] & 0x7F),
      .tm_min  = BCD_TO_DEC(rx[1] & 0x7F),
      .tm_hour = BCD_TO_DEC(rx[2] & 0x3F),
      .tm_mday = BCD_TO_DEC(rx[3] & 0x3F),
      .tm_mon  = BCD_TO_DEC(rx[5] & 0x1F) - 1,
      .tm_year = BCD_TO_DEC(rx[6]) + 100  // from 2000
    };

    time_t seconds = mktime(&tm);

    if (seconds == (time_t)-1) {
      STATE_ESPLOG(STATE_ERR, "RTC", "no valid time set");
    }

    struct timeval tv = {
      .tv_sec  = seconds,
      .tv_usec = esp_timer_get_time() % 1000000,
    };

    settimeofday(&tv, NULL);
  } else {
    STATE_ESPLOG(STATE_ERR, "RTC", "no valid time set");
  }
}

/*******************************************************************************
 * sdcard_init(): init SDIO, mount filesystem and create task
 ******************************************************************************/
static void sdcard_init(struct timeval *tv) {
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
    STATE_ESPLOG(STATE_FATAL, "SDCARD", "mount failure");
  }

  // set log file
  char logpath[64];
  struct tm *tm = localtime(&tv->tv_sec);
  strftime(logpath, sizeof(logpath), "/sdcard/%Y-%m-%d-%H-%M-%S.log", tm);

  int fd = open(logpath, O_RDWR | O_CREAT | O_TRUNC, 0);

  if (fd < 0) {
    STATE_ESPLOG(STATE_FATAL, "SDCARD", "log open failure");
  }

  // create log queue and sdcard task
  logqueue = xQueueCreate(32, sizeof(log_t));

  if (xTaskCreatePinnedToCore(task_sdcard, "sdcard", 4096, (void *)fd, 7, NULL, 0) != pdPASS) {
    STATE_ESPLOG(STATE_FATAL, "SDCARD", "task create failure");
  }
}

/*******************************************************************************
 * peripheral_task_init(): create peripheral recorder tasks
 ******************************************************************************/
static void peripheral_task_init(void) {
  nvs_handle_t nvs;

  if (nvs_open("enabled", NVS_READWRITE, &nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NVS", "enabled config open failure", "NVS_OPEN_FAIL");
  }

  uint8_t enabled = TRUE;

  /***** CAN *****/
  if (nvs_get_u8(nvs, "CAN", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "CAN", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_can, "can", 4096, NULL, 5, NULL, 1) != pdPASS) {
      STATE_SYSLOG(STATE_ERR, "CAN", "task create failure", "CAN_NEWTASK_FAIL");
    }
  }

  /***** GPS *****/
  if (nvs_get_u8(nvs, "GPS", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "GPS", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_gps, "gps", 4096, NULL, 5, NULL, 1) != pdPASS) {
      STATE_SYSLOG(STATE_ERR, "GPS", "task create failure", "GPS_NEWTASK_FAIL");
    }
  }

  /***** ANALOG *****/
  if (nvs_get_u8(nvs, "ANALOG", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "ANALOG", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_analog, "analog", 4096, NULL, 5, NULL, 1) != pdPASS) {
      STATE_SYSLOG(STATE_ERR, "ANALOG", "task create failure", "ANL_NEWTASK_FAIL");
    }
  }

  /***** DIGITAL *****/
  if (nvs_get_u8(nvs, "DIGITAL", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "DIGITAL", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_digital, "digital", 4096, NULL, 5, NULL, 1) != pdPASS) {
      STATE_SYSLOG(STATE_ERR, "DIGITAL", "task create failure", "DGT_NEWTASK_FAIL");
    }
  }

  /***** GYROSCOPE *****/
  if (nvs_get_u8(nvs, "GYRO", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "GYRO", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_gyroscope, "gyroscope", 4096, NULL, 5, NULL, 1) != pdPASS) {
      STATE_SYSLOG(STATE_ERR, "GYRO", "task create failure", "GYR_NEWTASK_FAIL");
    }
  }

  if (nvs_commit(nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NVS", "enabled config commit failure", "NVS_COMMIT_FAIL");
  }

  nvs_close(nvs);
}

/*******************************************************************************
 * network configuration reset button handler
 ******************************************************************************/
static void IRAM_ATTR reset_isr(void *arg) {
  // TODO: reset AP logic
}

/*******************************************************************************
 * system status LED indicator
 ******************************************************************************/
static void task_led(void *pvParameters) {
  int32_t led   = TRUE;
  state_t state = STATE_OK;
  BaseType_t ret;

  while (TRUE) {
    do {
      ret = xQueueReceive(ledqueue, &state, 0);
    } while (ret == pdTRUE);

    led = !led;
    gpio_set_level(GPIO_NUM_5, led);

    vTaskDelay(pdMS_TO_TICKS(state));
  }
}

/*******************************************************************************
 * save log queue to SD card every 1000 ms
 ******************************************************************************/
static void task_sdcard(void *pvParameters) {
  int fd = (int)pvParameters;
  log_t log;
  BaseType_t ret;

  while (TRUE) {
    do {
      ret = xQueueReceive(logqueue, &log, 0);

      if (ret == pdTRUE) {
        write(fd, &log, sizeof(log));
      }
    } while (ret == pdTRUE);

    fsync(fd);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/*******************************************************************************
 * create log entry
 ******************************************************************************/
BaseType_t LOG(uint8_t type, log_t *log) {
  // won't validate pointer and logqueue
  uint32_t *ptr   = (uint32_t *)log;
  uint32_t chksum = 0;

  // set log header
  log->magic     = LOG_MAGIC;
  log->checksum  = 0;
  log->type      = type;
  log->timestamp = (uint32_t)(esp_timer_get_time() / 1000);

  // calculate checksum
  for (size_t i = 0; i < sizeof(log_t) / sizeof(uint32_t); i++) {
    chksum ^= ptr[i];
  }

  // fold to 16 bit
  log->checksum = (chksum & 0xFFFF) + (chksum >> 16);

  return xQueueSend(logqueue, log, 0);
}
