#include <sys/time.h>
#include <time.h>

#include "main.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_mac.h"
#include "nvs_flash.h"

struct timeval boot;

nvs_handle_t nvs;
TaskHandle_t led;
QueueHandle_t logqueue;

state_t init = 0;
state_t run  = ALL_ERROR_FATAL;

const char components[][8] = { "CORE", "NVS", "RTC", "SD", "WIFI", "MQTT", "CAN", "GPS", "ANALOG", "DIGITAL", "GYRO" };

void sdcard_init(void);
void network_init(void);
static void rtc_init(void);
static void peripheral_task_init(void);

void task_can(void *pvParameters);
void task_gps(void *pvParameters);
void task_analog(void *pvParameters);
void task_digital(void *pvParameters);
void task_gyroscope(void *pvParameters);

static void reset_isr(void *arg);
static void task_led(void *pvParameters);

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

  if (gpio_config(&gpio) != ESP_OK || xTaskCreatePinnedToCore(task_led, "led", 2048, NULL, 5, &led, CORE0) != pdPASS) {
    ERROR_LOG(&init, CORE, "LED config failure");
  }

  /*** RST ***/
  gpio.pin_bit_mask = (1ULL << GPIO_NUM_21);
  gpio.mode         = GPIO_MODE_INPUT;
  gpio.intr_type    = GPIO_INTR_ANYEDGE;
  gpio.pull_down_en = GPIO_PULLDOWN_ENABLE;

  esp_err_t ret = gpio_config(&gpio);
  ret |= gpio_install_isr_service(0);
  ret |= gpio_isr_handler_add(GPIO_NUM_21, reset_isr, NULL);

  if (ret != ESP_OK) {
    ERROR_LOG(&init, CORE, "RST config failure");
  }

  /*** NVS ***/
  if (nvs_flash_init() != ESP_OK) {
    FATAL_LOG(&init, NVS, "NVS flash init failure");
  }

  if (nvs_open("storage", NVS_READWRITE, &nvs) != ESP_OK) {
    FATAL_LOG(&init, NVS, "open failure");
  }

  if (IS_OK(&init, NVS)) {
    CLEAR_ALL(&run, NVS);
  } else {
    COPY_STATE(&run, &init, NVS);
  }

  /*** RTC ***/
  rtc_init();

  if (IS_OK(&init, RTC)) {
    CLEAR_ALL(&run, RTC);
  } else {
    COPY_STATE(&run, &init, RTC);
  }

  // read boot time
  gettimeofday(&boot, NULL);

  /*** SDIO ***/
  sdcard_init();

  log_t boot_record;
  boot_record.payload.boot.protocol_version = PROTOCOL_VERSION;
  boot_record.payload.boot.boot_time        = (uint64_t)boot.tv_sec;
  esp_read_mac(boot_record.payload.boot.mac, ESP_MAC_WIFI_STA);

  if (LOG(LOG_TYPE_BOOT, &boot_record) != pdTRUE) {
    FATAL_LOG(&init, SD, "boot record failure");
  }

  if (IS_OK(&init, SD)) {
    CLEAR_ALL(&run, SD);
  } else {
    COPY_STATE(&run, &init, SD);
  }

  /*** peripherals ***/
  peripheral_task_init();

  /*** Wi-Fi ***/
  network_init();

  if (IS_OK(&init, WIFI)) {
    CLEAR_ALL(&run, WIFI);
    SYSLOG("WIFI_RDY");
  } else {
    COPY_STATE(&run, &init, WIFI);
  }

  if (IS_OK(&init, CORE)) {
    CLEAR_ALL(&run, CORE);
    SYSLOG("CORE_RDY");
  } else {
    COPY_STATE(&run, &init, CORE);
  }

  INFO(CORE, "initialization complete");
}

/*******************************************************************************
 * configuration reset button handler
 ******************************************************************************/
static void reset_isr(void *arg) {
  static int64_t press = 0;

  if (gpio_get_level(GPIO_NUM_21) == TRUE) {
    press = esp_timer_get_time();
  } else if (esp_timer_get_time() - press > 3000000) {
    nvs_erase_all(nvs);
    nvs_commit(nvs);
    esp_restart();
  }
}

/*******************************************************************************
 * system status LED indicator
 ******************************************************************************/
static void task_led(void *pvParameters) {
  uint32_t led_state            = TRUE;
  state_led_interval_t interval = STATE_OK;

  while (TRUE) {
    if (run & (COMPONENT_ALL << COMPONENT_MAX)) {
      interval = STATE_FATAL;
    } else if (run & COMPONENT_ALL) {
      interval = STATE_ERROR;
    } else {
      interval = STATE_OK;
    }

    led_state = !led_state;
    gpio_set_level(GPIO_NUM_5, led_state);

    INFO(CORE, "state: 0x%06lx", run);
    vTaskDelay(pdMS_TO_TICKS(interval));
  }
}

/*******************************************************************************
 * init RTC and set system time
 ******************************************************************************/
static void rtc_init(void) {
  i2c_master_bus_handle_t i2c0;
  i2c_master_bus_config_t i2c_config = {
    .clk_source                   = I2C_CLK_SRC_DEFAULT,
    .i2c_port                     = I2C_NUM_0,
    .scl_io_num                   = GPIO_NUM_10,
    .sda_io_num                   = GPIO_NUM_9,
    .glitch_ignore_cnt            = 7,
    .flags.enable_internal_pullup = false,
  };

  if (i2c_new_master_bus(&i2c_config, &i2c0) != ESP_OK) {
    ERROR_LOG(&init, RTC, "I2C init failure");
    return;
  }

  i2c_master_dev_handle_t rtc;
  i2c_device_config_t rtc_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x51,
    .scl_speed_hz    = 100000,
  };

  if (i2c_master_bus_add_device(i2c0, &rtc_cfg, &rtc) != ESP_OK) {
    ERROR_LOG(&init, RTC, "device init failure");
    return;
  }

  uint8_t tx[1] = { 0x02 };  // VL_seconds register address
  uint8_t rx[7] = { 0 };     // 0x02 VL_seconds to 0x08 Years register

  if (i2c_master_transmit_receive(rtc, tx, sizeof(tx), rx, sizeof(rx), 10) != ESP_OK) {
    i2c_master_bus_rm_device(rtc);
    ERROR_LOG(&init, RTC, "read time transfer failure");
    return;
  }

  i2c_master_bus_rm_device(rtc);

  // rtc has valid time set
  if (rx[6] != 0x00) {
    struct tm tm = {
      .tm_sec  = BCD_TO_DEC(rx[0] & 0x7F),
      .tm_min  = BCD_TO_DEC(rx[1] & 0x7F),
      .tm_hour = BCD_TO_DEC(rx[2] & 0x3F),
      .tm_mday = BCD_TO_DEC(rx[3] & 0x3F),
      .tm_wday = BCD_TO_DEC(rx[4] & 0x07),
      .tm_mon  = BCD_TO_DEC(rx[5] & 0x1F) - 1,
      .tm_year = BCD_TO_DEC(rx[6]) + 100  // from 2000
    };

    time_t seconds = mktime(&tm);

    if (seconds == (time_t)-1) {
      ERROR_LOG(&init, RTC, "mktime failure");
      return;
    }

    struct timeval tv = {
      .tv_sec  = seconds,
      .tv_usec = esp_timer_get_time() % 1000000,
    };

    settimeofday(&tv, NULL);
    INFO(RTC, "time set to %s", ctime(&tv.tv_sec));
  } else {
    ERROR_LOG(&init, RTC, "no valid time set");
    return;
  }
}

/*******************************************************************************
 create peripheral recorder tasks
 ******************************************************************************/
static void peripheral_task_init(void) {
  uint8_t enabled = TRUE;

  /***** CAN *****/
  if (nvs_get_u8(nvs, "can_en", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "can_en", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_can, "can", 4096, NULL, 5, NULL, CORE1) != pdPASS) {
      ERROR_SYSLOG(&init, CORE, "CAN task create failure", "CAN_TASK_FAIL");
    }
  }

  /***** GPS *****/
  if (nvs_get_u8(nvs, "gps_en", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "gps_en", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_gps, "gps", 4096, NULL, 5, NULL, CORE1) != pdPASS) {
      ERROR_SYSLOG(&init, CORE, "GPS task create failure", "GPS_TASK_FAIL");
    }
  }

  /***** ANALOG *****/
  if (nvs_get_u8(nvs, "anl_en", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "anl_en", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_analog, "analog", 4096, NULL, 5, NULL, CORE1) != pdPASS) {
      ERROR_SYSLOG(&init, CORE, "ANALOG task create failure", "ANL_TASK_FAIL");
    }
  }

  /***** DIGITAL *****/
  if (nvs_get_u8(nvs, "dgt_en", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "dgt_en", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_digital, "digital", 4096, NULL, 5, NULL, CORE1) != pdPASS) {
      ERROR_SYSLOG(&init, CORE, "DIGITAL task create failure", "DGT_TASK_FAIL");
    }
  }

  /***** GYROSCOPE (always enabled) *****/
  if (xTaskCreatePinnedToCore(task_gyroscope, "gyroscope", 4096, NULL, 5, NULL, CORE1) != pdPASS) {
    ERROR_SYSLOG(&init, CORE, "GYROSCOPE task create failure", "GYR_TASK_FAIL");
  }

  if (nvs_commit(nvs) != ESP_OK) {
    ERROR_SYSLOG(&run, NVS, "commit failure", "NVS_COMMIT_FAIL");
  }
}
