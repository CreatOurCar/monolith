#include "main.h"

#include "driver/twai.h"

/*******************************************************************************
 * CAN traffic monitor / transmitter task
 ******************************************************************************/
void task_can(void *pvParameters) {
  // TODO: check bps, filter

  if (nvs_commit(nvs) != ESP_OK) {
    ERROR_SYSLOG(&run, NVS, "commit failure: can", "CAN_NVS_FAIL");
  }

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_7, GPIO_NUM_6, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config  = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config  = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK || twai_start() != ESP_OK) {
    ERROR_SYSLOG(&init, CAN, "driver init failure", "CAN_INIT_FAIL");
  }

  // TODO: set can ISR

  if (IS_OK(&init, CAN)) {
    CLEAR_ALL(&run, CAN);
    SYSLOG("CAN_RDY");
  } else {
    COPY_STATE(&run, &init, CAN);
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (TRUE) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}

