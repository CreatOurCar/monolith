#include "main.h"

void task_can(void *pvParameters) {
  // TODO: check CAN enabled
  // TODO: check send enabled, bps, filter
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_7, GPIO_NUM_6, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    // TODO: error handling
  }

  if (twai_start() != ESP_OK) {
    // TODO: error handling
  }

  // TOOD: set can ISR
}

void task_gps(void *pvParameters) {}

void task_analog(void *pvParameters) {
  // TODO: read nvs config

  i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = GPIO_NUM_10,
    .sda_io_num = GPIO_NUM_9,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = false,
  };

  i2c_master_bus_handle_t i2c0_handle;

  if (i2c_new_master_bus(&i2c_mst_config, &i2c0_handle) != ESP_OK) {
    // TODO: error handling
  }

  i2c_device_config_t rtc_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x51,
    .scl_speed_hz = 100000,
  };

  i2c_master_dev_handle_t dev_handle;

  if (i2c_master_bus_add_device(i2c0_handle, &rtc_cfg, &dev_handle) != ESP_OK) {
    // TODO: error handling
  }

  while (TRUE) {
    vTaskDelay(pdMS_TO_TICKS(100));
    // TODO: read analog values
  }
}

void task_digital(void *pvParameters) {
  gpio_config_t gpio;

  // TODO: check enabled channels
  gpio.pin_bit_mask = (1ULL << GPIO_NUM_11) | (1ULL << GPIO_NUM_12) | (1ULL << GPIO_NUM_13) | (1ULL << GPIO_NUM_14);
  gpio.mode = GPIO_MODE_INPUT;
  gpio.intr_type = GPIO_INTR_ANYEDGE;
  gpio.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio.pull_down_en = GPIO_PULLDOWN_ENABLE;

  if (gpio_config(&gpio) != ESP_OK) {
    // TODO: error handling
  }

  // TODO: set ISR
}

void task_gyroscope(void *pvParameters) {
  i2c_master_bus_handle_t i2c0_handle;

  // i2c0 already initalized by main thread
  if (i2c_master_get_bus_handle(I2C_NUM_0, &i2c0_handle) != ESP_OK) {
    // TODO: error handling
  }

  i2c_device_config_t gyro_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x68,
    .scl_speed_hz = 100000,
  };

  i2c_master_dev_handle_t dev_handle;

  if (i2c_master_bus_add_device(i2c0_handle, &gyro_cfg, &dev_handle) != ESP_OK) {
    // TODO: error handling
  }
}

void task_temperature(void *pvParameters) {
  temperature_sensor_handle_t sensor = NULL;
  temperature_sensor_config_t sensor_cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);

  if (temperature_sensor_install(&sensor_cfg, &sensor) != ESP_OK) {
    // TODO: error handling
  }

  if (temperature_sensor_enable(sensor) != ESP_OK) {
    // TODO: error handling
  }

  float temperature;

  while (TRUE) {
    if (temperature_sensor_get_celsius(sensor, &temperature) != ESP_OK) {
      // TODO: error handling
    }

    // TODO: record

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
