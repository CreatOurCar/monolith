#include "main.h"

#include "driver/i2c_master.h"
#include "driver/temperature_sensor.h"

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
    ERROR_SYSLOG(ANALOG, "I2C init failure", "ANL_INIT_FAIL");
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

  esp_err_t ret = i2c_master_bus_add_device(i2c1_handle, &adc0_cfg, &dev_handle);
  ret |= i2c_master_bus_add_device(i2c1_handle, &adc1_cfg, &dev_handle);

  if (ret != ESP_OK) {
    ERROR_SYSLOG(ANALOG, "device init failure", "ANL_DEV_FAIL");
  }

  temperature_sensor_handle_t sensor     = NULL;
  temperature_sensor_config_t sensor_cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);

  if (temperature_sensor_install(&sensor_cfg, &sensor) != ESP_OK || temperature_sensor_enable(sensor) != ESP_OK) {
    ERROR_SYSLOG(ANALOG, "temperature sensor init failure", "TMP_INIT_FAIL");
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (TRUE) {
    float temperature;

    if (temperature_sensor_get_celsius(sensor, &temperature) != ESP_OK) {
      // TODO: error handling
    }

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}
