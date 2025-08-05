#include "main.h"

#include "driver/gpio.h"
#include "driver/uart.h"

/*******************************************************************************
 * GPS NMEA GPRMC message monitor task
 ******************************************************************************/
void task_gps(void *pvParameters) {
  QueueHandle_t uart_queue;
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

  if (IS_OK(&init, GPS)) {
    CLEAR_ALL(&logbuf.run, GPS);
    SYSLOG("GPS_RDY");
  } else {
    COPY_STATE(&logbuf.run, &init, GPS);
  }

  while (true) {
    uart_event_t event;

    if (xQueueReceive(uart_queue, &event, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    uint8_t data[1024];

    if (event.type == UART_PATTERN_DET) {
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

      printf("%.*s", len, data);
      uart_pattern_queue_reset(UART_NUM_1, 16);
    }
  }
}
