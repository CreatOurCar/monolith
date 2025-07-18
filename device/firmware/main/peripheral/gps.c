#include "main.h"

/*******************************************************************************
 * GPS NMEA GPRMC message monitor task
 ******************************************************************************/
void task_gps(void *pvParameters) {
  // TODO: impl

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
