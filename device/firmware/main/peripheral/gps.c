#include "main.h"

/*******************************************************************************
 * GPS NMEA GPRMC message monitor task
 ******************************************************************************/
void task_gps(void *pvParameters) {
  if (IS_OK(&init, GPS)) {
    CLEAR_ALL(&logbuf.run, GPS);
    SYSLOG("GPS_RDY");
  } else {
    COPY_STATE(&logbuf.run, &init, GPS);
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (true) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}
