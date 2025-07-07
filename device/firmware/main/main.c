#include "main.h"

state_t system_state         = STATE_OK;
TaskHandle_t task_led_handle = NULL;

QueueHandle_t logqueue = NULL;

static void reset_isr(void *arg);
static void task_led(void *pvParameters);
static void task_sdcard(void *pvParameters);

void app_main(void) {
  // core system initialization
  gpio_config_t gpio;

  /*** LED ********************************************************************/
  gpio.pin_bit_mask = (1ULL << GPIO_NUM_5);
  gpio.mode         = GPIO_MODE_OUTPUT_OD;
  gpio.intr_type    = GPIO_INTR_DISABLE;
  gpio.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;

  if (gpio_config(&gpio) != ESP_OK) {
    ESP_LOGW("LED", "GPIO config failure");
  }

  if (xTaskCreatePinnedToCore(task_led, "led", 1024, NULL, 5, &task_led_handle, 0) != pdPASS) {
    ESP_LOGW("LED", "task creation failure");
  }

  /*** RST ********************************************************************/
  gpio.pin_bit_mask = (1ULL << GPIO_NUM_21);
  gpio.mode         = GPIO_MODE_INPUT;
  gpio.intr_type    = GPIO_INTR_ANYEDGE;
  gpio.pull_down_en = GPIO_PULLDOWN_ENABLE;

  if (gpio_config(&gpio) != ESP_OK) {
    ESP_LOGW("RST", "GPIO config failure");
  }

  if (gpio_install_isr_service(0) != ESP_OK) {
    ESP_LOGW("RST", "ISR service install failure");
  }

  if (gpio_isr_handler_add(GPIO_NUM_21, reset_isr, NULL) != ESP_OK) {
    ESP_LOGW("RST", "ISR handler add failure");
  }

  /*** NVS ********************************************************************/
  if (nvs_flash_init() != ESP_OK) {
    SET_STATE(STATE_FATAL);
    ESP_LOGE("NVS", "flash init failure");
  }

  /*** RTC ********************************************************************/
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
    SET_STATE(STATE_ERR);
    ESP_LOGW("RTC", "I2C0 init failure");
  }

  i2c_master_dev_handle_t rtc_handle;
  i2c_device_config_t rtc_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x51,
    .scl_speed_hz    = 100000,
  };

  if (i2c_master_bus_add_device(i2c0_handle, &rtc_cfg, &rtc_handle) != ESP_OK) {
    SET_STATE(STATE_ERR);
    ESP_LOGW("RTC", "device init failure");
  }

  uint8_t tx[1] = { 0x02 };  // VL_seconds register address
  uint8_t rx[7] = { 0 };     // 0x02 VL_seconds to 0x08 Years register

  if (i2c_master_transmit_receive(rtc_handle, tx, sizeof(tx), rx, sizeof(rx), 100) != ESP_OK) {
    SET_STATE(STATE_ERR);
    memset(rx, 0, sizeof(rx));
    ESP_LOGW("RTC", "read time transfer failure");
  }

  struct timeval tv;
  struct tm tm = { 0 };

  // rtc has valid time set
  if (rx[6] != 0x00) {
    tm.tm_sec  = BCD_TO_DEC(rx[0] & 0x7F);
    tm.tm_min  = BCD_TO_DEC(rx[1] & 0x7F);
    tm.tm_hour = BCD_TO_DEC(rx[2] & 0x3F);
    tm.tm_mday = BCD_TO_DEC(rx[3] & 0x3F);
    tm.tm_mon  = BCD_TO_DEC(rx[5] & 0x1F) - 1;
    tm.tm_year = BCD_TO_DEC(rx[6]) + 100;  // from 2000

    time_t seconds = mktime(&tm);

    if (seconds == (time_t)-1) {
      SET_STATE(STATE_ERR);
      ESP_LOGW("RTC", "mktime failure");
    }

    gettimeofday(&tv, NULL);
    tv.tv_sec += seconds;
    settimeofday(&tv, NULL);
  }

  /*** SDIO *******************************************************************/
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
    SET_STATE(STATE_FATAL);
    ESP_LOGE("SDCARD", "mount failure");
  }

  gettimeofday(&tv, NULL);
  struct tm *tm_info = localtime(&tv.tv_sec);

  char logpath[64];
  strftime(logpath, sizeof(logpath), "/sdcard/%Y-%m-%d-%H-%M-%S.log", tm_info);

  int fd = open(logpath, O_RDWR | O_CREAT | O_TRUNC, 0);

  if (fd < 0) {
    SET_STATE(STATE_FATAL);
    ESP_LOGE("SDCARD", "log file open failure");
  }

  logqueue = xQueueCreate(32, sizeof(log_t));

  // TODO: add boot record
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  // halt on fatal error
  // if (system_state == STATE_FATAL) {
  //   ESP_LOGE("CORE LOGIC", "Fatal error during initialization. halting...");
  //   while (TRUE);
  // }

  // Core 0: sdcard sync task
  xTaskCreatePinnedToCore(task_sdcard, "sdcard", 4096, (void *)fd, 7, NULL, 0);

  // Core 0: network task
  xTaskCreatePinnedToCore(task_network, "network", 4096, NULL, 5, NULL, 0);

  // Core 1: data record tasks
  xTaskCreatePinnedToCore(task_can, "can", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(task_gps, "gps", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(task_analog, "analog", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(task_digital, "digital", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(task_gyroscope, "gyroscope", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(task_temperature, "temperature", 4096, NULL, 5, NULL, 1);
}

static void IRAM_ATTR reset_isr(void *arg) {
  // TODO: reset AP logic
}

static void task_led(void *pvParameters) {
  static int32_t value = FALSE;

  while (TRUE) {
    gpio_set_level(GPIO_NUM_5, value);
    value = !value;
    vTaskDelay(pdMS_TO_TICKS(system_state));
  }
}

static void task_sdcard(void *pvParameters) {
  int fd = (int)pvParameters;
  log_t log;

  while (TRUE) {
    BaseType_t ret;

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
