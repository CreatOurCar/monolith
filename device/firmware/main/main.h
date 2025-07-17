#ifndef MAIN_H
#define MAIN_H

#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "nvs.h"

#define TRUE (1)
#define FALSE (0)

/***** shared global variables *****/
extern char ssid[32];
extern char passwd[32];
extern char server[64];
extern char name[32];
extern char key[32];
extern uint8_t mac[6];
extern uint32_t name_len;

extern QueueHandle_t logqueue;
extern nvs_handle_t nvs;
extern uint32_t state;
extern EventGroupHandle_t led;

extern esp_mqtt_client_handle_t mqtt;

enum {
  MQTT_QOS_0,
  MQTT_QOS_1,
  MQTT_QOS_2,
};

/***** system state *****/
extern const char components[][8];

typedef enum {
  CORE,
  NVS,
  RTC,
  SD,
  WIFI,
  MQTT,
  CAN,
  GPS,
  ANALOG,
  DIGITAL,
  GYRO,
} state_component_t;

typedef enum {
  STATE_OK    = 1000,
  STATE_ERROR = 250,
  STATE_FATAL = 100,
} state_led_interval_t;

static inline void SET_ERROR(state_component_t component) {
  state |= (1 << component);
  xEventGroupSetBits(led, 1);
}

static inline void SET_FATAL(state_component_t component) {
  state |= (1 << (component + 12));
  xEventGroupSetBits(led, 1);
}

static inline void CLEAR_ERROR(state_component_t component) {
  state &= ~(1 << component);
  xEventGroupClearBits(led, 1);
}

static inline void CLEAR_FATAL(state_component_t component) {
  state &= ~(1 << (component + 12));
  xEventGroupClearBits(led, 1);
}

#define IS_ERROR(component) (state & (1 << component))
#define IS_FATAL(component) (state & (1 << (component + 12)))

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
  LOG_TYPE_ANALOG,
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
  int16_t ain1;
  int16_t ain2;
  int16_t ain3;
  int16_t ain4;
  int16_t ain5;
  int16_t ain6;
  int16_t voltage;
  int16_t temperature;
} analog_record_t;

typedef struct {
  uint8_t channel;
  uint8_t value;
  uint8_t _reserved[14];
} digital_record_t;

typedef struct {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t temperature;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
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

static inline int LOG(uint8_t type, log_t *log) {
  uint32_t *ptr   = (uint32_t *)log;
  uint32_t chksum = 0;

  // set log header
  log->magic     = LOG_MAGIC;
  log->checksum  = 0;
  log->type      = type;
  log->timestamp = (uint32_t)(esp_timer_get_time() / 1000);

  // calculate checksum
  for (size_t i = 0; i < sizeof(log_t) / sizeof(uint32_t); i++) {
    chksum ^= ptr[i];
  }

  // fold to 16 bit
  log->checksum = (chksum & 0xFFFF) + (chksum >> 16);

  return xQueueSend(logqueue, log, 0);
}

static inline void SYSLOG(const char *msg) {
  log_t log;
  strncpy(log.payload.system_event.msg, msg, sizeof(log.payload.system_event.msg));  // no need to null-terminate
  LOG(LOG_TYPE_SYSTEM, &log);
}

static inline void ERROR_LOG(state_component_t component, const char *msg) {
  SET_ERROR(component);
  ESP_LOGW(components[component], "%s", msg);
}

static inline void FATAL_LOG(state_component_t component, const char *msg) {
  SET_FATAL(component);
  ESP_LOGE(components[component], "%s", msg);
}

static inline void ERROR_SYSLOG(state_component_t component, const char *msg, const char *log) {
  SYSLOG(log);
  ERROR_LOG(component, msg);
}

static inline void FATAL_SYSLOG(state_component_t component, const char *msg, const char *log) {
  SYSLOG(log);
  FATAL_LOG(component, msg);
}

/***** peripheral config *****/
enum {
  CAN_BPS_1K,
  CAN_BPS_5K,
  CAN_BPS_10K,
  CAN_BPS_12_5K,
  CAN_BPS_16K,
  CAN_BPS_20K,
  CAN_BPS_25K,
  CAN_BPS_50K,
  CAN_BPS_100K,
  CAN_BPS_125K,
  CAN_BPS_250K,
  CAN_BPS_500K,
  CAN_BPS_800K,
  CAN_BPS_1M,
  CAN_BPS_MAX,
};

enum {
  GPS_DEV_UBLOX,
  GPS_DEV_MAX,
};

/***** utility functions *****/
#define INFO(component, fmt, ...) ESP_LOGI(components[component], fmt, ##__VA_ARGS__)

static inline int BCD_TO_DEC(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }
static inline uint8_t DEC_TO_BCD(int dec) { return ((dec / 10) << 4) | (dec % 10); }

#endif  // MAIN_H
