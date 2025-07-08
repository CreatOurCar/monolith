#include "main.h"

/*******************************************************************************
 * CAN traffic monitor / transmitter task
 ******************************************************************************/
void task_can(void *pvParameters) {
  nvs_handle_t nvs;

  if (nvs_open("CAN", NVS_READWRITE, &nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "CAN", "nvs open failure", "CAN_NVS_FAIL");
  }

  // TODO: check send enabled, bps, filter

  if (nvs_commit(nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "CAN", "nvs commit failure", "NVS_COMMIT_FAIL");
  }

  nvs_close(nvs);

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_7, GPIO_NUM_6, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config  = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config  = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK || twai_start() != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "CAN", "driver init failure", "CAN_INIT_FAIL");
  }

  // TODO: set can ISR

  while (TRUE) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/*******************************************************************************
 * GPS NMEA GPRMC message monitor task
 ******************************************************************************/
void task_gps(void *pvParameters) {
  nvs_handle_t nvs;

  if (nvs_open("GPS", NVS_READWRITE, &nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "GPS", "nvs open failure", "GPS_NVS_FAIL");
  }

  // TODO: check GPS conf, set uart and ISR

  if (nvs_commit(nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "CAN", "nvs commit failure", "NVS_COMMIT_FAIL");
  }

  nvs_close(nvs);

  while (TRUE) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/*******************************************************************************
 * Analog channel and temperature sensor monitor task
 ******************************************************************************/
void task_analog(void *pvParameters) {
  i2c_master_bus_config_t i2c_mst_config = {
    .clk_source                   = I2C_CLK_SRC_DEFAULT,
    .i2c_port                     = I2C_NUM_1,
    .scl_io_num                   = GPIO_NUM_48,
    .sda_io_num                   = GPIO_NUM_47,
    .glitch_ignore_cnt            = 7,
    .flags.enable_internal_pullup = false,
  };

  i2c_master_bus_handle_t i2c1_handle;

  if (i2c_new_master_bus(&i2c_mst_config, &i2c1_handle) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "ANALOG", "I2C1 init failure", "ANL_INIT_FAIL");
  }

  i2c_device_config_t adc0_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x48,
    .scl_speed_hz    = 100000,
  };

  i2c_device_config_t adc1_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x49,
    .scl_speed_hz    = 100000,
  };

  i2c_master_dev_handle_t dev_handle;

  if (i2c_master_bus_add_device(i2c1_handle, &adc0_cfg, &dev_handle) != ESP_OK ||
      i2c_master_bus_add_device(i2c1_handle, &adc1_cfg, &dev_handle) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "ANALOG", "device init failure", "ANL_DEV_FAIL");
  }

  temperature_sensor_handle_t sensor     = NULL;
  temperature_sensor_config_t sensor_cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);

  if (temperature_sensor_install(&sensor_cfg, &sensor) != ESP_OK || temperature_sensor_enable(sensor) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "TEMP", "sensor init failure", "TMP_INIT_FAIL");
  }

  float temperature;

  while (TRUE) {
    // TODO: record

    if (temperature_sensor_get_celsius(sensor, &temperature) != ESP_OK) {
      // TODO: error handling
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/*******************************************************************************
 * Digital input interrupt monitor task
 ******************************************************************************/
void task_digital(void *pvParameters) {
  gpio_config_t gpio;

  gpio.pin_bit_mask = (1ULL << GPIO_NUM_11) | (1ULL << GPIO_NUM_12) | (1ULL << GPIO_NUM_13) | (1ULL << GPIO_NUM_14);
  gpio.mode         = GPIO_MODE_INPUT;
  gpio.intr_type    = GPIO_INTR_ANYEDGE;
  gpio.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpio.pull_down_en = GPIO_PULLDOWN_ENABLE;

  if (gpio_config(&gpio) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "DIGITAL", "GPIO init failure", "DGT_INIT_FAIL");
  }

  // TODO: set ISR

  while (TRUE) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/*******************************************************************************
 * Gyroscope monitor task
 ******************************************************************************/
void task_gyroscope(void *pvParameters) {
  i2c_master_bus_handle_t i2c0_handle;

  // i2c0 already initalized by main thread
  if (i2c_master_get_bus_handle(I2C_NUM_0, &i2c0_handle) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "GYRO", "I2C0 get bus failure", "GYR_INIT_FAIL");
  }

  i2c_device_config_t gyro_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x68,
    .scl_speed_hz    = 100000,
  };

  i2c_master_dev_handle_t dev_handle;

  if (i2c_master_bus_add_device(i2c0_handle, &gyro_cfg, &dev_handle) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "GYRO", "device init failure", "GYR_DEV_FAIL");
  }

  while (TRUE) {
    vTaskDelay(pdMS_TO_TICKS(100));
    // TODO: read analog values
  }
}
