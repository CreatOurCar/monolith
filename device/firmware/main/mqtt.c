#include "main.h"

esp_mqtt_client_handle_t mqtt = NULL;
extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");

#define STREQL(str1, str2) (strcmp((str1), (str2)) == 0)

static void mqtt_handle_data(esp_mqtt_event_handle_t evt) {
  char topic[64];
  snprintf(topic, sizeof(topic), "%.*s", evt->topic_len, evt->topic);

  char *dir[5]  = { 0 };  // name, cmd, type, key
  char *saveptr = NULL;
  int cnt       = 0;

  for (char *tok = strtok_r(topic, "/", &saveptr); tok && cnt < 4; tok = strtok_r(NULL, "/", &saveptr)) {
    dir[cnt++] = tok;
  }

  if (cnt < 2 || !STREQL(dir[0], name)) {
    return;
  }

  if (STREQL(dir[1], "set")) {
    if (cnt < 4) {
      return;
    }

    // TODO: impl

    if (STREQL(dir[2], "net")) {
      if (STREQL(dir[3], "ssid")) {
      } else if (STREQL(dir[3], "passwd")) {
      }
    } else if (STREQL(dir[2], "can")) {
      if (STREQL(dir[3], "en")) {
      } else if (STREQL(dir[3], "bps")) {
      } else if (STREQL(dir[3], "filter")) {
      }
    } else if (STREQL(dir[2], "gps")) {
      if (STREQL(dir[3], "en")) {
      } else if (STREQL(dir[3], "dev")) {
      }
    } else if (STREQL(dir[2], "anl")) {
      if (STREQL(dir[3], "en")) {
      }
    } else if (STREQL(dir[2], "dgt")) {
      if (STREQL(dir[3], "en")) {
      }
    } else if (STREQL(dir[2], "gyr")) {
      if (STREQL(dir[3], "en")) {
      }
    }
  }

  else if (STREQL(dir[1], "cmd")) {
    if (dir[2] == NULL) {
      return;
    }

    if (STREQL(dir[2], "can")) {
      // TODO: transmit CAN message
    } else if (STREQL(dir[2], "ls")) {
      // TODO: list all files
    } else if (STREQL(dir[2], "del")) {
      // TODO: delete file(s)
    } else if (STREQL(dir[2], "get")) {
      // TODO: download file
    } else if (STREQL(dir[2], "evt")) {
      log_t log;
      strncpy(log.payload.user_event.msg, evt->data, sizeof(log.payload.user_event.msg));
      LOG(LOG_TYPE_USER_EVENT, &log);
    } else if (STREQL(dir[2], "rbt")) {
      SYSLOG("MQTT_REBOOT");
      vTaskDelay(pdMS_TO_TICKS(3000));
      esp_restart();
    }
  }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;

  switch (event_id) {
    char buf[16];
    char topic[40];

    case MQTT_EVENT_CONNECTED:
      snprintf(topic, sizeof(topic), "%s/set/#", name);
      esp_mqtt_client_subscribe(mqtt, topic, 2);
      snprintf(topic, sizeof(topic), "%s/cmd/#", name);
      esp_mqtt_client_subscribe(mqtt, topic, 1);
      CLEAR_ERROR(MQTT);
      SYSLOG("MQTT_CONN");
      break;
    case MQTT_EVENT_DISCONNECTED:
      ERROR_SYSLOG(MQTT, "disconnected", "MQTT_DISCONN");
      break;
    case MQTT_EVENT_DATA:
      mqtt_handle_data(event);
      break;
    case MQTT_EVENT_ERROR:
      snprintf(buf, sizeof(buf), "MQTT_ERR:%d", event->error_handle->error_type);
      ERROR_SYSLOG(MQTT, buf, buf);
      break;
    default:
      break;
  }
}

void mqtt_client(void) {
  char mqtt_url[80];
  snprintf(mqtt_url, sizeof(mqtt_url), "wss://%s:443", server);

  esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri                  = mqtt_url,
    .broker.verification.certificate     = (const char *)isrgrootx1_pem_start,
    .credentials.username                = name,
    .credentials.authentication.password = key,
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
