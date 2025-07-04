#ifndef MAIN_H
#define MAIN_H

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_vfs_fat.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/sdmmc_host.h"
#include "driver/temperature_sensor.h"
#include "driver/twai.h"

#include "nvs.h"
#include "nvs_flash.h"

#define TRUE (1)
#define FALSE (0)

typedef enum {
  STATE_OK = 3000,
  STATE_ERR = 500,
  STATE_FATAL = 100,
} state_t;

extern state_t system_state;
extern TaskHandle_t task_led_handle;

static inline void SET_STATE(state_t state) {
  system_state = state;
  xTaskAbortDelay(task_led_handle);
}

static inline int BCD_TO_DEC(uint8_t bcd) {
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static inline uint8_t DEC_TO_BCD(int dec) {
  return ((dec / 10) << 4) | (dec % 10);
}

void task_can(void *pvParameters);
void task_gps(void *pvParameters);
void task_analog(void *pvParameters);
void task_digital(void *pvParameters);
void task_gyroscope(void *pvParameters);
void task_temperature(void *pvParameters);

void task_network(void *pvParameters);

#endif  // MAIN_H
