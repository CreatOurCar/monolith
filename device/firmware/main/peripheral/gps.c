#include "main.h"

/*******************************************************************************
 * GPS NMEA GPRMC message monitor task
 ******************************************************************************/
void task_gps(void *pvParameters) {
  // TODO: check GPS conf, set uart and ISR

  if (nvs_commit(nvs) != ESP_OK) {
    ERROR_SYSLOG(GPS, "commit failure: GPS", "GPS_NVS_FAIL");
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (TRUE) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}

