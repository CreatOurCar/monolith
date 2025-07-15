#include "main.h"

esp_mqtt_client_handle_t mqtt = NULL;

extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t isrgrootx1_pem_end[] asm("_binary_isrgrootx1_pem_end");

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
  esp_mqtt_event_handle_t event = event_data;

  switch (event_id) {
    case MQTT_EVENT_CONNECTED:
      esp_mqtt_client_subscribe(event->client, "monolith/+/set", 0);
      CLEAR_ERROR(MQTT);
      SYSLOG("MQTT_CONN");
      break;
    case MQTT_EVENT_DISCONNECTED:
      ERROR_SYSLOG(MQTT, "disconnected", "MQTT_DISCONN");
      break;
    case MQTT_EVENT_DATA:
      // Handle incoming data here

      break;
    default:
      break;
  }
}

void mqtt_client(void) {
  char mqtt_url[80];
  snprintf(mqtt_url, sizeof(mqtt_url), "wss://%s:443", server);

  esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri              = mqtt_url,
    .broker.verification.certificate = (const char*)isrgrootx1_pem_start,
  };

  mqtt = esp_mqtt_client_init(&mqtt_cfg);

  if (mqtt == NULL) {
    ERROR_SYSLOG(MQTT, "client create failure", "MQTT_INIT_FAIL");
    return;
  }

  esp_mqtt_client_register_event(mqtt, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

  if (esp_mqtt_client_start(mqtt) != ESP_OK) {
    ERROR_SYSLOG(MQTT, "client start failure", "MQTT_START_FAIL");
    esp_mqtt_client_destroy(mqtt);
    mqtt = NULL;
    return;
  }
}

