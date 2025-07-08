#ifndef MAIN_H
#define MAIN_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
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

/***** system state (LED blink interval) *****/
typedef enum {
  STATE_OK    = 3000,
  STATE_ERR   = 500,
  STATE_FATAL = 100,
} state_t;

/***** shared global variables *****/
extern state_t system_state;
extern TaskHandle_t task_led_handle;
extern QueueHandle_t logqueue;

/***** log protocol *****/
#define PROTOCOL_VERSION 1
#define LOG_MAGIC 0xAE

typedef enum {
  LOG_TYPE_INVALID,
  LOG_TYPE_BOOT,
  LOG_TYPE_SYSTEM,
  LOG_TYPE_USER_EVENT,
  LOG_TYPE_CAN,
  LOG_TYPE_GPS,
  LOG_TYPE_ANALOG_1234,
  LOG_TYPE_ANALOG_56VT,
  LOG_TYPE_DIGITAL,
  LOG_TYPE_GYROSCOPE,
} log_type_t;

typedef struct {
  uint8_t protocol_version;
  uint8_t _reserved[1];
  uint8_t mac[6];
  uint64_t boot_time;  // second since epoch
} boot_record_t;

typedef struct {
  uint32_t id;
  uint8_t len;
  uint8_t _reserved[3];
  uint8_t data[8];
} can_record_t;

typedef struct {
  float latitude;
  float longitude;
  uint8_t lat_dir;  // 0: N, 1: S
  uint8_t lon_dir;  // 0: E, 1: W
  uint8_t _reserved[2];
  uint16_t speed;
  uint16_t course;
} gps_record_t;

typedef struct {
  float data1;
  float data2;
  float data3;
  float data4;
} analog_record_t;

typedef struct {
  uint8_t channel;
  uint8_t value;
  uint8_t _reserved[14];
} digital_record_t;

typedef struct {
  uint16_t accel_x;
  uint16_t accel_y;
  uint16_t accel_z;
  uint16_t gyro_x;
  uint16_t gyro_y;
  uint16_t gyro_z;
  uint16_t temperature;
  uint8_t _reserved[2];
} gyroscope_record_t;

typedef struct {
  char msg[16];
} system_event_t;

typedef system_event_t user_event_t;

typedef struct {
  uint8_t magic;
  uint8_t type;
  uint16_t checksum;
  uint32_t timestamp;
  union {
    boot_record_t boot;
    can_record_t can;
    gps_record_t gps;
    analog_record_t analog;
    digital_record_t digital;
    gyroscope_record_t gyroscope;
    system_event_t system_event;
    user_event_t user_event;
  } payload;  // 16 bytes
} log_t;

/***** utility functions *****/
static inline void SET_STATE(state_t state) {
  system_state = state;
  xTaskAbortDelay(task_led_handle);
}

static inline int BCD_TO_DEC(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }
static inline uint8_t DEC_TO_BCD(int dec) { return ((dec / 10) << 4) | (dec % 10); }

/***** function prototypes *****/
BaseType_t LOG(uint8_t type, log_t *log);

void task_can(void *pvParameters);
void task_gps(void *pvParameters);
void task_analog(void *pvParameters);
void task_digital(void *pvParameters);
void task_gyroscope(void *pvParameters);

void task_network(void *pvParameters);

#endif  // MAIN_H
