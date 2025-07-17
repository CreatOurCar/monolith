#include "main.h"

/*******************************************************************************
 * GPS NMEA GPRMC message monitor task
 ******************************************************************************/
void task_gps(void *pvParameters) {
  // TODO: check GPS conf, set uart and ISR

  if (nvs_commit(nvs) != ESP_OK) {
    ERROR_SYSLOG(&run, NVS, "commit failure: gps", "GPS_NVS_FAIL");
  }

  if (IS_OK(&init, GPS)) {
    CLEAR_ALL(&run, GPS);
    SYSLOG("GPS_RDY");
  } else {
    COPY_STATE(&run, &init, GPS);
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (TRUE) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}
