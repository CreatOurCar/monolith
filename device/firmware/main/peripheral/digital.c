#include "main.h"

#include "driver/gpio.h"

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
    ERROR_SYSLOG(DIGITAL, "GPIO init failure", "DGT_INIT_FAIL");
  }

  // TODO: set ISR
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (TRUE) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}


