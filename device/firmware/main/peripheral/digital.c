#include "main.h"

#include "driver/gpio.h"

static void din1_isr(void *arg);
static void din2_isr(void *arg);
static void din3_isr(void *arg);
static void din4_isr(void *arg);

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

  esp_err_t ret = gpio_config(&gpio);
  ret |= gpio_isr_handler_add(GPIO_NUM_11, din1_isr, NULL);
  ret |= gpio_isr_handler_add(GPIO_NUM_12, din2_isr, NULL);
  ret |= gpio_isr_handler_add(GPIO_NUM_13, din3_isr, NULL);
  ret |= gpio_isr_handler_add(GPIO_NUM_14, din4_isr, NULL);

  if (ret != ESP_OK) {
    ERROR_SYSLOG(&init, DIGITAL, "GPIO init failure", "DGT_INIT_FAIL");
  }

  if (IS_OK(&init, DIGITAL)) {
    CLEAR_ALL(&run, DIGITAL);
    SYSLOG("DGT_RDY");
  } else {
    COPY_STATE(&run, &init, DIGITAL);
  }

  while (TRUE) {
    // TODO: publish digital state to MQTT
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void din1_isr(void *arg) {
  log_t log;
  log.payload.digital.channel = 1;
  log.payload.digital.state   = gpio_get_level(GPIO_NUM_11);
  LOG(LOG_TYPE_DIGITAL, &log);
}

static void din2_isr(void *arg) {
  log_t log;
  log.payload.digital.channel = 2;
  log.payload.digital.state   = gpio_get_level(GPIO_NUM_12);
  LOG(LOG_TYPE_DIGITAL, &log);
}

static void din3_isr(void *arg) {
  log_t log;
  log.payload.digital.channel = 3;
  log.payload.digital.state   = gpio_get_level(GPIO_NUM_13);
  LOG(LOG_TYPE_DIGITAL, &log);
}

static void din4_isr(void *arg) {
  log_t log;
  log.payload.digital.channel = 4;
  log.payload.digital.state   = gpio_get_level(GPIO_NUM_14);
  LOG(LOG_TYPE_DIGITAL, &log);
}
