#include "main.h"

#include "driver/i2c_master.h"
#include "driver/temperature_sensor.h"

#define ADS1115_CONVERSION_REG_ADDR 0x00
#define ADS1115_CONFIG_REG_ADDR 0x01

#define ADS1115_MUX_AIN0 (0b100 << 4)
#define ADS1115_MUX_AIN1 (0b101 << 4)
#define ADS1115_MUX_AIN2 (0b110 << 4)
#define ADS1115_MUX_AIN3 (0b111 << 4)
#define ADS1115_OS (1 << 7)

#define ADS1115_CONFIG_H 0x05  // +-2.048V FSR, single-shot mode
#define ADS1115_CONFIG_L 0xE3  // 860SPS (1.16ms per conversion)
#define ADS1115_CONFIG(channel) (ADS1115_CONFIG_H | ADS1115_OS | channel)

#define VOLT(v) ((float)(v) * 2.048 / (1 << 15))

static esp_timer_handle_t timer;
static EventGroupHandle_t i2c_evt;
static EventGroupHandle_t timer_evt;

void IRAM_ATTR timer_isr(void *arg) { xEventGroupSetBits(timer_evt, BIT(1)); }

esp_err_t convert(i2c_master_dev_handle_t adc, uint8_t ch, int16_t *v);
esp_err_t convert_2(i2c_master_dev_handle_t adc1, i2c_master_dev_handle_t adc2, uint8_t ch, int16_t *v1, int16_t *v2);

static bool adc1_cb(i2c_master_dev_handle_t dev, const i2c_master_event_data_t *evt_data, void *arg) {
  xEventGroupSetBits(i2c_evt, BIT(1));
  return true;
}

static bool adc2_cb(i2c_master_dev_handle_t dev, const i2c_master_event_data_t *evt_data, void *arg) {
  xEventGroupSetBits(i2c_evt, BIT(2));
  return true;
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
    .trans_queue_depth            = 2,
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

  i2c_master_event_callbacks_t adc1_cbs = { .on_trans_done = adc1_cb };
  i2c_master_event_callbacks_t adc2_cbs = { .on_trans_done = adc2_cb };

  ret |= i2c_master_register_event_callbacks(adc1, &adc1_cbs, NULL);
  ret |= i2c_master_register_event_callbacks(adc2, &adc2_cbs, NULL);

  if (ret != ESP_OK) {
    i2c_del_master_bus(i2c1);
    ERROR_SYSLOG(&init, ANALOG, "device init failure", "ANL_DEV_FAIL");
    vTaskDelete(NULL);
  }

  esp_timer_create_args_t timer_args = {
    .callback        = timer_isr,
    .dispatch_method = ESP_TIMER_TASK,
  };

  if (esp_timer_create(&timer_args, &timer) != ESP_OK) {
    ERROR_SYSLOG(&init, ANALOG, "timer create failure", "TIMER_CR_FAIL");
  }

  i2c_evt   = xEventGroupCreate();
  timer_evt = xEventGroupCreate();

  if (IS_OK(&init, ANALOG)) {
    CLEAR_ALL(&run, ANALOG);
    SYSLOG("ANL_RDY");
  } else {
    COPY_STATE(&run, &init, ANALOG);
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (TRUE) {
    log_t log;
    float temperature;

    esp_err_t err = convert(adc1, ADS1115_MUX_AIN3, &log.payload.analog.ain4);
    err |= convert_2(adc1, adc2, ADS1115_MUX_AIN0, &log.payload.analog.ain1, &log.payload.analog.ain5);
    err |= convert_2(adc1, adc2, ADS1115_MUX_AIN1, &log.payload.analog.ain2, &log.payload.analog.ain6);
    err |= convert_2(adc1, adc2, ADS1115_MUX_AIN2, &log.payload.analog.ain3, &log.payload.analog.voltage);

    if (err != ESP_OK) {
      ERROR_SYSLOG(&run, ANALOG, "ADC read failure", "ADC_READ_FAIL");
    }

    if (temperature_sensor_get_celsius(sensor, &temperature) != ESP_OK) {
      ERROR_SYSLOG(&run, ANALOG, "temperature sensor read failure", "TMP_READ_FAIL");
    }

    log.payload.analog.temperature = (int16_t)(temperature * 100);
    LOG(ANALOG, &log);

    INFO(ANALOG, "%f V, %f V, %f V, %f V, %f V, %f V, %f V, %f C", VOLT(log.payload.analog.ain1),
      VOLT(log.payload.analog.ain2), VOLT(log.payload.analog.ain3), VOLT(log.payload.analog.ain4),
      VOLT(log.payload.analog.ain5), VOLT(log.payload.analog.ain6), VOLT(log.payload.analog.voltage),
      (float)log.payload.analog.temperature / 100);

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}

esp_err_t convert(i2c_master_dev_handle_t adc, uint8_t ch, int16_t *v) {
  uint8_t tx_conv = ADS1115_CONVERSION_REG_ADDR;
  uint8_t tx[3]   = { ADS1115_CONFIG_REG_ADDR, ADS1115_CONFIG(ch), ADS1115_CONFIG_L };
  uint8_t rx[2]   = { 0 };

  esp_err_t ret = i2c_master_transmit(adc, tx, sizeof(tx), 10);
  xEventGroupWaitBits(i2c_evt, BIT(1), TRUE, TRUE, portMAX_DELAY);

  // wat for conversion to finish
  esp_timer_start_once(timer, 1200);
  xEventGroupWaitBits(timer_evt, BIT(1), TRUE, FALSE, portMAX_DELAY);

  ret |= i2c_master_transmit_receive(adc, &tx_conv, sizeof(tx_conv), rx, sizeof(rx), 10);
  xEventGroupWaitBits(i2c_evt, BIT(1), TRUE, TRUE, portMAX_DELAY);

  *v = ((int16_t)rx[0] << 8) | rx[1];

  return ret;
}

esp_err_t convert_2(i2c_master_dev_handle_t adc1, i2c_master_dev_handle_t adc2, uint8_t ch, int16_t *v1, int16_t *v2) {
  uint8_t tx_conv = ADS1115_CONVERSION_REG_ADDR;
  uint8_t tx[3]   = { ADS1115_CONFIG_REG_ADDR, ADS1115_CONFIG(ch), ADS1115_CONFIG_L };
  uint8_t rx1[2]  = { 0 };
  uint8_t rx2[2]  = { 0 };

  esp_err_t ret = i2c_master_transmit(adc1, tx, sizeof(tx), 10);
  ret |= i2c_master_transmit(adc2, tx, sizeof(tx), 10);
  xEventGroupWaitBits(i2c_evt, BIT(1) | BIT(2), TRUE, TRUE, portMAX_DELAY);

  esp_timer_start_once(timer, 1200);
  xEventGroupWaitBits(timer_evt, BIT(1), TRUE, FALSE, portMAX_DELAY);

  ret |= i2c_master_transmit_receive(adc1, &tx_conv, sizeof(tx_conv), rx1, sizeof(rx1), 10);
  ret |= i2c_master_transmit_receive(adc2, &tx_conv, sizeof(tx_conv), rx2, sizeof(rx2), 10);
  xEventGroupWaitBits(i2c_evt, BIT(1) | BIT(2), TRUE, TRUE, portMAX_DELAY);

  *v1 = ((int16_t)rx1[0] << 8) | rx1[1];
  *v2 = ((int16_t)rx2[0] << 8) | rx2[1];

  return ret;
}
