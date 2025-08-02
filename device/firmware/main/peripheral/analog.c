#include "main.h"

#include "driver/i2c_master.h"
#include "driver/temperature_sensor.h"

#define ADS1115_CONVERSION_REG_ADDR 0x00
#define ADS1115_CONFIG_REG_ADDR 0x01

#define MUX_AIN0 (0b100 << 4)
#define MUX_AIN1 (0b101 << 4)
#define MUX_AIN2 (0b110 << 4)
#define MUX_AIN3 (0b111 << 4)
#define ADS1115_OS (1 << 7)

#define ADS1115_CONFIG_H 0x03  // +-4.096V FSR, single-shot mode
#define ADS1115_CONFIG_L 0xE3  // 860SPS (1.16ms per conversion)
#define ADS1115_CONFIG(channel) (ADS1115_CONFIG_H | ADS1115_OS | channel)

#define VOLT(v) ((int)(((v) * 2048) >> 15))

esp_err_t convert(i2c_master_dev_handle_t adc, uint8_t ch, int16_t *v) {
  uint8_t tx_conv = ADS1115_CONVERSION_REG_ADDR;
  uint8_t tx[3]   = { ADS1115_CONFIG_REG_ADDR, ADS1115_CONFIG(ch), ADS1115_CONFIG_L };
  uint8_t rx[2]   = { 0 };

  esp_err_t ret = i2c_master_transmit(adc, tx, sizeof(tx), 20);
  esp_rom_delay_us(1200);  // wait for conversion complete
  ret |= i2c_master_transmit_receive(adc, &tx_conv, sizeof(tx_conv), rx, sizeof(rx), 10);

  *v = ((int16_t)rx[0] << 8) | rx[1];

  return ret;
}

/*******************************************************************************
 * Analog channel and temperature sensor monitor task
 ******************************************************************************/
void task_analog(void *pvParameters) {
  // initialize temperature sensor
  temperature_sensor_handle_t sensor     = NULL;
  temperature_sensor_config_t sensor_cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);

  if (temperature_sensor_install(&sensor_cfg, &sensor) != ESP_OK || temperature_sensor_enable(sensor) != ESP_OK) {
    ERROR_SYSLOG(&init, ANALOG, "temperature sensor init failure", "TMP_INIT_FAIL");
  }

  // initialize i2c bus and adc
  i2c_master_bus_handle_t i2c1;
  i2c_master_bus_config_t i2c_config = {
    .clk_source                   = I2C_CLK_SRC_DEFAULT,
    .i2c_port                     = I2C_NUM_1,
    .scl_io_num                   = GPIO_NUM_48,
    .sda_io_num                   = GPIO_NUM_47,
    .glitch_ignore_cnt            = 7,
    .flags.enable_internal_pullup = false,
  };

  if (i2c_new_master_bus(&i2c_config, &i2c1) != ESP_OK) {
    ERROR_SYSLOG(&init, ANALOG, "I2C init failure", "ANL_INIT_FAIL");
    vTaskDelete(NULL);
  }

  i2c_master_dev_handle_t adc1;
  i2c_device_config_t adc1_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x48,
    .scl_speed_hz    = 100000,
  };

  i2c_master_dev_handle_t adc2;
  i2c_device_config_t adc2_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x49,
    .scl_speed_hz    = 100000,
  };

  esp_err_t ret = i2c_master_bus_add_device(i2c1, &adc1_cfg, &adc1);
  ret |= i2c_master_bus_add_device(i2c1, &adc2_cfg, &adc2);

  if (ret != ESP_OK) {
    i2c_del_master_bus(i2c1);
    ERROR_SYSLOG(&init, ANALOG, "device init failure", "ANL_DEV_FAIL");
    vTaskDelete(NULL);
  }

  if (IS_OK(&init, ANALOG)) {
    CLEAR_ALL(&logbuf.run, ANALOG);
    SYSLOG("ANL_RDY");
  } else {
    COPY_STATE(&logbuf.run, &init, ANALOG);
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();
  float temperature;
  log_t analog;

  while (true) {
    esp_err_t err = convert(adc1, MUX_AIN0, &analog.payload.analog.ain1);
    err |= convert(adc1, MUX_AIN1, &analog.payload.analog.ain2);
    err |= convert(adc1, MUX_AIN2, &analog.payload.analog.ain3);
    err |= convert(adc1, MUX_AIN3, &analog.payload.analog.ain4);
    err |= convert(adc2, MUX_AIN0, &analog.payload.analog.ain5);
    err |= convert(adc2, MUX_AIN1, &analog.payload.analog.ain6);
    err |= convert(adc2, MUX_AIN2, &analog.payload.analog.voltage);
    err |= temperature_sensor_get_celsius(sensor, &temperature);

    if (err == ESP_OK) {
      analog.payload.analog.temperature = (int16_t)(temperature * 100);
      LOG(LOG_TYPE_ANALOG, &analog);
      memcpy(&logbuf.analog, &analog, sizeof(log_t));
    } else {
      ERROR_SYSLOG(&logbuf.run, ANALOG, "ADC read failure", "ADC_READ_FAIL");
    }

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}
