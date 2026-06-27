#ifndef MAIN_H
#define MAIN_H

#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "nvs.h"

#include "config.h"

enum { CORE0, CORE1 };

/***** shared global variables *****/
extern nvs_handle_t nvs;
extern TaskHandle_t led;
extern QueueHandle_t logqueue;
extern QueueHandle_t syslogqueue;
extern QueueHandle_t canlogqueue;
extern QueueHandle_t cantxqueue;
extern esp_mqtt_client_handle_t mqtt;
extern volatile bool file_op_busy;

/***** nvs storage *****/
typedef struct {
  struct {
    uint8_t mac[6];
    char macaddr[18];
    char ssid[32];
    char passwd[32];
  } wifi;
  struct {
    char server[64];
    char name[32];
    char key[32];
    char tz[40];
    uint32_t intv;
  } device;
  struct {
    uint8_t can;
    uint8_t gps;
    uint8_t analog;
    uint8_t digital;
  } enabled;
  struct {
    uint8_t bps;
    uint8_t _reserved[3];
    uint32_t filter;
    uint32_t mask;
  } can;
  struct {
    uint8_t dev;
    uint8_t _reserved[3];
  } gps;
} nvs_storage_t;

extern nvs_storage_t storage;

/***** system state *****/
typedef uint32_t state_t;
extern state_t init;
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
  COMPONENT_MAX = 12,
  COMPONENT_ALL = 0x07FF,
} state_component_t;

#define ALL_ERROR_FATAL (COMPONENT_ALL | (COMPONENT_ALL << COMPONENT_MAX))

typedef enum {
  STATE_OK    = pdMS_TO_TICKS(1000),
  STATE_ERROR = pdMS_TO_TICKS(250),
  STATE_FATAL = pdMS_TO_TICKS(100),
} state_led_interval_t;

static inline void SET_ERROR(state_t *state, state_component_t component) {
  *state |= (1 << component);
  xTaskAbortDelay(led);
}

static inline void SET_FATAL(state_t *state, state_component_t component) {
  *state |= (1 << (component + COMPONENT_MAX));
  xTaskAbortDelay(led);
}

static inline void CLEAR_ERROR(state_t *state, state_component_t component) {
  *state &= ~(1 << component);
  xTaskAbortDelay(led);
}

static inline void CLEAR_FATAL(state_t *state, state_component_t component) {
  *state &= ~(1 << (component + COMPONENT_MAX));
  xTaskAbortDelay(led);
}

static inline void CLEAR_ALL(state_t *state, state_component_t component) {
  *state &= ~((1 << component) | (1 << (component + COMPONENT_MAX)));
  xTaskAbortDelay(led);
}

static inline void COPY_STATE(state_t *dest, state_t *src, state_component_t component) {
  *dest = (*dest & ~(1 << component)) | (*src & (1 << component));
  *dest = (*dest & ~(1 << (component + COMPONENT_MAX))) | (*src & (1 << (component + COMPONENT_MAX)));
  xTaskAbortDelay(led);
}

#define IS_ERROR(state, component) (*state & (1 << component))
#define IS_FATAL(state, component) (*state & (1 << (component + COMPONENT_MAX)))
#define IS_OK(state, component) (!IS_ERROR(state, component) && !IS_FATAL(state, component))

/***** log protocol (wire/storage data layout — see protocol.h) *****/
#include "protocol.h"

typedef struct {
  uint32_t timestamp;
  state_t run;
  log_t gps;
  log_t gyro;
  log_t analog;
  log_t digital;
} log_buf_t;

extern log_buf_t logbuf;

static inline void log_prepare(uint8_t type, log_t *log) {
  uint32_t *ptr   = (uint32_t *)log;
  uint32_t chksum = 0;

  // set log header
  log->magic     = LOG_MAGIC;
  log->type      = type;
  log->checksum  = 0;
  log->timestamp = (uint32_t)(esp_timer_get_time() / 1000);

  // calculate checksum
  for (size_t i = 0; i < sizeof(log_t) / sizeof(uint32_t); i++) {
    chksum ^= ptr[i];
  }

  // fold to 16 bit
  log->checksum = (chksum & 0xFFFF) + (chksum >> 16);
}

static inline int LOG(uint8_t type, log_t *log) {
  if (logqueue == NULL) return 0;
  log_prepare(type, log);
  return xQueueSend(logqueue, log, 0);
}

static inline int LOG_FROM_ISR(uint8_t type, log_t *log) {
  if (logqueue == NULL) return 0;
  log_prepare(type, log);
  return xQueueSendFromISR(logqueue, log, NULL);
}

static inline void SYSLOG(const char *msg) {
  if (logqueue == NULL || syslogqueue == NULL) return;
  log_t log;
  strncpy(log.payload.system_event.msg, msg, sizeof(log.payload.system_event.msg));  // no need to null-terminate
  LOG(LOG_TYPE_SYSTEM, &log);
  xQueueSend(syslogqueue, &log, 0);
}

static inline void ERROR_LOG(state_t *state, state_component_t component, const char *msg) {
  SET_ERROR(state, component);
  ESP_LOGW(components[component], "%s", msg);
}

static inline void FATAL_LOG(state_t *state, state_component_t component, const char *msg) {
  SET_FATAL(state, component);
  ESP_LOGE(components[component], "%s", msg);
}

static inline void ERROR_SYSLOG(state_t *state, state_component_t component, const char *msg, const char *log) {
  SYSLOG(log);
  ERROR_LOG(state, component, msg);
}

static inline void FATAL_SYSLOG(state_t *state, state_component_t component, const char *msg, const char *log) {
  SYSLOG(log);
  FATAL_LOG(state, component, msg);
}

/***** utility functions *****/
#define INFO(component, fmt, ...) ESP_LOGI(components[component], fmt, ##__VA_ARGS__)

/***** display CAN data snapshot (written by task_can, read by task_display) *****/
typedef struct {
  uint16_t   ez_rpm_raw;   // EZ 0x180117EF B6-B7 LE; rpm = raw * 0.1 - 2000
  uint16_t   bms_soc_raw;  // Daly 0x18904001 B6-B7 BE; SOC % = raw * 0.1
  uint8_t    valid;
  TickType_t last_tick;
} display_can_t;

extern volatile display_can_t display_can;

#endif  // MAIN_H
