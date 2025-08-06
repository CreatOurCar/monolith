#include "main.h"

#include "driver/gpio.h"
#include "driver/uart.h"

QueueHandle_t uart_queue;

void init_ublox(void) {
  uart_config_t uart_config = {
    .baud_rate = 9600,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };

  if (uart_driver_install(UART_NUM_1, 2048, 256, 16, &uart_queue, 0) != ESP_OK ||
      uart_param_config(UART_NUM_1, &uart_config) != ESP_OK ||
      uart_set_pin(UART_NUM_1, GPIO_NUM_17, GPIO_NUM_18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK ||
      uart_enable_pattern_det_baud_intr(UART_NUM_1, '\n', 1, 10, 0, 0) != ESP_OK) {
    ERROR_SYSLOG(&init, GPS, "UART driver init failure", "GPS_UART_FAIL");
  }

  const uint8_t GPS_DISABLE_NMEA_GxGGA[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x24 };
  const uint8_t GPS_DISABLE_NMEA_GxGLL[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x2B };
  const uint8_t GPS_DISABLE_NMEA_GxGSA[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x32 };
  const uint8_t GPS_DISABLE_NMEA_GxGSV[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x03, 0x39 };
  const uint8_t GPS_DISABLE_NMEA_GxVTG[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x05, 0x47 };

  const uint8_t GPS_PMS_FULL[]  = { 0xB5, 0x62, 0x06, 0x86, 0x00, 0x00, 0x8C, 0xAA };
  const uint8_t GPS_RATE_10HZ[] = { 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A,
    0x12 };

  if (uart_write_bytes(UART_NUM_1, GPS_DISABLE_NMEA_GxGGA, sizeof(GPS_DISABLE_NMEA_GxGGA)) < 0 ||
      uart_write_bytes(UART_NUM_1, GPS_DISABLE_NMEA_GxGLL, sizeof(GPS_DISABLE_NMEA_GxGLL)) < 0 ||
      uart_write_bytes(UART_NUM_1, GPS_DISABLE_NMEA_GxGSA, sizeof(GPS_DISABLE_NMEA_GxGSA)) < 0 ||
      uart_write_bytes(UART_NUM_1, GPS_DISABLE_NMEA_GxGSV, sizeof(GPS_DISABLE_NMEA_GxGSV)) < 0 ||
      uart_write_bytes(UART_NUM_1, GPS_DISABLE_NMEA_GxVTG, sizeof(GPS_DISABLE_NMEA_GxVTG)) < 0 ||
      uart_write_bytes(UART_NUM_1, GPS_PMS_FULL, sizeof(GPS_PMS_FULL)) < 0 ||
      uart_write_bytes(UART_NUM_1, GPS_RATE_10HZ, sizeof(GPS_RATE_10HZ)) < 0) {
    ERROR_SYSLOG(&init, GPS, "module config failure", "GPS_CFG_FAIL");
  }

  uart_flush_input(UART_NUM_1);
}

/*******************************************************************************
 * GPS NMEA GPRMC message monitor task
 ******************************************************************************/
void task_gps(void *pvParameters) {
  switch (storage.gps.dev) {
    case GPS_DEV_UBLOX:
      init_ublox();
      break;

    default:
      ERROR_SYSLOG(&init, GPS, "unknown device", "GPS_DEV_UNKNOWN");
      break;
  }

  if (IS_OK(&init, GPS)) {
    CLEAR_ALL(&logbuf.run, GPS);
    SYSLOG("GPS_RDY");
  } else {
    COPY_STATE(&logbuf.run, &init, GPS);
  }

  uint8_t data[256];
  uart_event_t event;

  while (true) {
    if (xQueueReceive(uart_queue, &event, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    if (event.type != UART_PATTERN_DET) {
      continue;
    }

    int pos = uart_pattern_pop_pos(UART_NUM_1);

    if (pos < 0) {
      uart_flush_input(UART_NUM_1);
      continue;
    }

    int len = uart_read_bytes(UART_NUM_1, data, pos + 1, 50);

    if (len < 0) {
      uart_flush_input(UART_NUM_1);
      continue;
    }

    if (strncmp((char *)data, "$GPRMC", 6) == 0) {
      printf("%.*s", len, data);
    }

    uart_pattern_queue_reset(UART_NUM_1, 16);
  }
}
