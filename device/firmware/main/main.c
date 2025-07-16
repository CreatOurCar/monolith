#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "main.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/sdmmc_host.h"
#include "esp_mac.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"

struct timeval boot;
QueueHandle_t logqueue;

uint32_t state;
EventGroupHandle_t led;

const char components[][8] = { "CORE", "NVS", "RTC", "SD", "WIFI", "MQTT", "CAN", "GPS", "ANALOG", "DIGITAL", "GYRO" };

void task_can(void *pvParameters);
void task_gps(void *pvParameters);
void task_analog(void *pvParameters);
void task_digital(void *pvParameters);
void task_gyroscope(void *pvParameters);
void task_network(void *pvParameters);

static void rtc_init(void);
static void sdcard_init(void);
static void peripheral_task_init(void);

static void reset_isr(void *arg);
static void task_led(void *pvParameters);
static void task_sdcard(void *pvParameters);

static inline int BCD_TO_DEC(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }
static inline uint8_t DEC_TO_BCD(int dec) { return ((dec / 10) << 4) | (dec % 10); }

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

  esp_err_t ret = gpio_config(&gpio);
  led           = xEventGroupCreate();

  if (ret != ESP_OK || xTaskCreatePinnedToCore(task_led, "led", 1024, NULL, 5, NULL, 0) != pdPASS) {
    ERROR_LOG(CORE, "LED config failure");
  }

  /*** RST ***/
  gpio.pin_bit_mask = (1ULL << GPIO_NUM_21);
  gpio.mode         = GPIO_MODE_INPUT;
  gpio.intr_type    = GPIO_INTR_ANYEDGE;
  gpio.pull_down_en = GPIO_PULLDOWN_ENABLE;

  ret = gpio_config(&gpio);
  ret |= gpio_install_isr_service(0);
  ret |= gpio_isr_handler_add(GPIO_NUM_21, reset_isr, NULL);

  if (ret != ESP_OK) {
    ERROR_LOG(CORE, "RST config failure");
  }

  /*** NVS ***/
  if (nvs_flash_init() != ESP_OK) {
    FATAL_LOG(NVS, "NVS flash init failure");
  }

  /*** RTC ***/
  rtc_init();

  // read boot time
  gettimeofday(&boot, NULL);

  /*** SDIO ***/
  sdcard_init();

  // save boot record
  log_t boot_record;
  boot_record.payload.boot.protocol_version = PROTOCOL_VERSION;
  boot_record.payload.boot.boot_time        = (uint64_t)boot.tv_sec;
  esp_read_mac(boot_record.payload.boot.mac, ESP_MAC_WIFI_STA);

  if (LOG(LOG_TYPE_BOOT, &boot_record) != pdTRUE) {
    FATAL_LOG(SD, "boot record failure");
  }

  /*** Wi-Fi ***/
  if (xTaskCreatePinnedToCore(task_network, "network", 8192, NULL, 5, NULL, 0) != pdPASS) {
    FATAL_SYSLOG(WIFI, "network task creation failure", "NET_TASK_FAIL");
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
    ERROR_LOG(RTC, "I2C init failure");
  }

  i2c_master_dev_handle_t rtc_handle;
  i2c_device_config_t rtc_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x51,
    .scl_speed_hz    = 100000,
  };

  if (i2c_master_bus_add_device(i2c0_handle, &rtc_cfg, &rtc_handle) != ESP_OK) {
    ERROR_LOG(RTC, "device init failure");
  }

  uint8_t tx[1] = { 0x02 };  // VL_seconds register address
  uint8_t rx[7] = { 0 };     // 0x02 VL_seconds to 0x08 Years register

  if (i2c_master_transmit_receive(rtc_handle, tx, sizeof(tx), rx, sizeof(rx), 100) != ESP_OK) {
    memset(rx, 0, sizeof(rx));
    ERROR_LOG(RTC, "read time transfer failure");
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
      ERROR_LOG(RTC, "mktime failure");
    }

    struct timeval tv = {
      .tv_sec  = seconds,
      .tv_usec = esp_timer_get_time() % 1000000,
    };

    settimeofday(&tv, NULL);
  } else {
    ERROR_LOG(RTC, "no valid time set");
  }
}

/*******************************************************************************
 * sdcard_init(): init SDIO, mount filesystem and create task
 ******************************************************************************/
static void sdcard_init(void) {
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
  struct tm *tm = localtime(&boot.tv_sec);
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
}

/*******************************************************************************
 * peripheral_task_init(): create peripheral recorder tasks
 ******************************************************************************/
static void peripheral_task_init(void) {
  nvs_handle_t nvs;

  if (nvs_open("enabled", NVS_READWRITE, &nvs) != ESP_OK) {
    ERROR_SYSLOG(NVS, "open failure: enabled", "NVS_OPEN_FAIL");
  }

  uint8_t enabled = TRUE;

  /***** CAN *****/
  if (nvs_get_u8(nvs, "CAN", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "CAN", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_can, "can", 4096, NULL, 5, NULL, 1) != pdPASS) {
      ERROR_SYSLOG(CAN, "task create failure", "CAN_TASK_FAIL");
    }
  }

  /***** GPS *****/
  if (nvs_get_u8(nvs, "GPS", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "GPS", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_gps, "gps", 4096, NULL, 5, NULL, 1) != pdPASS) {
      ERROR_SYSLOG(GPS, "task create failure", "GPS_TASK_FAIL");
    }
  }

  /***** ANALOG *****/
  if (nvs_get_u8(nvs, "ANALOG", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "ANALOG", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_analog, "analog", 4096, NULL, 5, NULL, 1) != pdPASS) {
      ERROR_SYSLOG(ANALOG, "task create failure", "ANL_TASK_FAIL");
    }
  }

  /***** DIGITAL *****/
  if (nvs_get_u8(nvs, "DIGITAL", &enabled) != ESP_OK) {
    nvs_set_u8(nvs, "DIGITAL", TRUE);
    enabled = TRUE;
  }

  if (enabled) {
    if (xTaskCreatePinnedToCore(task_digital, "digital", 4096, NULL, 5, NULL, 1) != pdPASS) {
      ERROR_SYSLOG(DIGITAL, "task create failure", "DGT_TASK_FAIL");
    }
  }

  /***** GYROSCOPE (always enabled) *****/
  if (xTaskCreatePinnedToCore(task_gyroscope, "gyroscope", 4096, NULL, 5, NULL, 1) != pdPASS) {
    ERROR_SYSLOG(GYRO, "task create failure", "GYR_TASK_FAIL");
  }

  if (nvs_commit(nvs) != ESP_OK) {
    ERROR_SYSLOG(NVS, "commit failure: enabled", "NVS_COMMIT_FAIL");
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
  int32_t led_state             = TRUE;
  state_led_interval_t interval = STATE_OK;

  while (TRUE) {
    xEventGroupWaitBits(led, 1, TRUE, FALSE, 0);

    if (state & (0xFFFF << 12)) {
      interval = STATE_FATAL;
    } else if (state & 0xFFFF) {
      interval = STATE_ERROR;
    } else {
      interval = STATE_OK;
    }

    led_state = !led_state;
    gpio_set_level(GPIO_NUM_5, led_state);

    vTaskDelay(pdMS_TO_TICKS(interval));
  }
}

/*******************************************************************************
 * save log queue to SD card every 1000 ms
 ******************************************************************************/
static void task_sdcard(void *pvParameters) {
  int fd = (int)pvParameters;
  int ret;
  log_t log;

  while (TRUE) {
    do {
      ret = xQueueReceive(logqueue, &log, 0);

      if (ret == pdTRUE) {
        write(fd, &log, sizeof(log));
      }
    } while (ret == pdTRUE);

    if (fsync(fd) != 0 && !(state & (1 << (SD + 16)))) {
      FATAL_LOG(SD, "fsync failure");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
