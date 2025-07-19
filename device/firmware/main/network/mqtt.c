#include <sys/time.h>

#include "main.h"

log_buf_t logbuf;
esp_mqtt_client_handle_t mqtt = NULL;

extern struct timeval boot;
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

  if (cnt < 2 || !STREQL(dir[0], storage.device.name) || evt->data_len == 0) {
    return;
  }

  // only update NVS and do not update memory; new values will be used on next boot
  if (STREQL(dir[1], "set")) {
    if (cnt < 4) {
      return;
    }

    if (STREQL(dir[2], "net")) {
      if (STREQL(dir[3], "ssid")) {
        char ssid[32];
        snprintf(ssid, sizeof(ssid), "%.*s", evt->data_len, evt->data);
        nvs_set_str(nvs, "ssid", ssid);
      } else if (STREQL(dir[3], "passwd")) {
        char passwd[32];
        snprintf(passwd, sizeof(passwd), "%.*s", evt->data_len, evt->data);
        nvs_set_str(nvs, "passwd", passwd);
      }
    }

    else if (STREQL(dir[2], "can")) {
      if (STREQL(dir[3], "en")) {
        nvs_set_u8(nvs, "can_en", evt->data[0] != 0);
      } else if (STREQL(dir[3], "bps")) {
        nvs_set_u8(nvs, "can_bps", evt->data[0]);
      } else if (STREQL(dir[3], "filter")) {
        nvs_set_u32(nvs, "can_filter", *(uint32_t *)(evt->data));
      } else if (STREQL(dir[3], "mask")) {
        nvs_set_u32(nvs, "can_mask", *(uint32_t *)(evt->data));
      }
    }

    else if (STREQL(dir[2], "gps")) {
      if (STREQL(dir[3], "en")) {
        nvs_set_u8(nvs, "gps_en", evt->data[0] != 0);
      } else if (STREQL(dir[3], "dev")) {
        nvs_set_u8(nvs, "gps_dev", evt->data[0]);
      }
    }

    else if (STREQL(dir[2], "anl")) {
      if (STREQL(dir[3], "en")) {
        nvs_set_u8(nvs, "anl_en", evt->data[0] != 0);
      }
    }

    else if (STREQL(dir[2], "dgt")) {
      if (STREQL(dir[3], "en")) {
        nvs_set_u8(nvs, "dgt_en", evt->data[0] != 0);
      }
    }

    if (nvs_commit(nvs) != ESP_OK) {
      ERROR_SYSLOG(&logbuf.run, NVS, "commit failure", "MQTT_NVS_FAIL");
      return;
    }

    char ack[40];
    snprintf(topic, sizeof(topic), "%s/ack", storage.device.name);
    esp_mqtt_client_publish(mqtt, ack, topic, 0, MQTT_QOS_2, false);
  }

  else if (STREQL(dir[1], "cmd")) {
    if (dir[2] == NULL) {
      return;
    }

    if (STREQL(dir[2], "rbt")) {
      esp_restart();
    } else if (STREQL(dir[2], "evt")) {
      // user event
      log_t log;
      strncpy(log.payload.user_event.msg, evt->data, sizeof(log.payload.user_event.msg));
      LOG(LOG_TYPE_USER_EVENT, &log);
    } else if (STREQL(dir[2], "ls")) {
      // list files
      // TODO:
    } else if (STREQL(dir[2], "del")) {
      // delete file(s)
      // TODO:
    } else if (STREQL(dir[2], "get")) {
      // download file
      // TODO:
    } else if (STREQL(dir[2], "can")) {
      // transmit CAN message
      // TODO:
    }
  }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;

  INFO(MQTT, "evt %ld topic: %.*s data: %.*s", event_id, event->topic_len, event->topic, event->data_len, event->data);

  switch (event_id) {
    char buf[sizeof(system_event_t)];
    char topic[40];

    case MQTT_EVENT_CONNECTED:
      snprintf(topic, sizeof(topic), "%s/set/#", storage.device.name);
      esp_mqtt_client_subscribe(mqtt, topic, MQTT_QOS_2);

      snprintf(topic, sizeof(topic), "%s/cmd/#", storage.device.name);
      esp_mqtt_client_subscribe(mqtt, topic, MQTT_QOS_2);

      snprintf(topic, sizeof(topic), "%s/d/boot", storage.device.name);
      esp_mqtt_client_publish(mqtt, topic, (char *)&boot.tv_sec, sizeof(boot.tv_sec), MQTT_QOS_1, true);

      snprintf(topic, sizeof(topic), "%s/d/cfg", storage.device.name);
      esp_mqtt_client_publish(mqtt, topic, (char *)&storage, sizeof(storage), MQTT_QOS_1, true);

      CLEAR_ALL(&logbuf.run, MQTT);
      SYSLOG("MQTT_CONN");
      break;
    case MQTT_EVENT_DISCONNECTED:
      ERROR_SYSLOG(&logbuf.run, MQTT, "disconnected", "MQTT_DISCONN");
      break;
    case MQTT_EVENT_DATA:
      mqtt_handle_data(event);
      break;
    case MQTT_EVENT_ERROR:
      snprintf(buf, sizeof(buf), "MQTT_ERR:%d", event->error_handle->error_type);
      ERROR_SYSLOG(&logbuf.run, MQTT, buf, buf);
      break;
    default:
      break;
  }
}

static void mqtt_task(void *arg) {
  while (TRUE) {
    if (mqtt != NULL && IS_OK(&logbuf.run, MQTT)) {
      // 1s frequency: core state, analog values, digital values, gps data, gyro data
      // on demand: can event
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void mqtt_init(void) {
  char mqtt_url[80];
  snprintf(mqtt_url, sizeof(mqtt_url), "wss://%s:443", storage.device.server);

  char topic[40];
  snprintf(topic, sizeof(topic), "%s/d/boot", storage.device.name);

  esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri                  = mqtt_url,
    .broker.verification.certificate     = (const char *)isrgrootx1_pem_start,
    .credentials.username                = storage.device.name,
    .credentials.authentication.password = storage.device.key,
    .session.last_will.topic             = topic,
    .session.last_will.msg               = "OFFLINE",
    .session.last_will.qos               = MQTT_QOS_1,
    .session.last_will.retain            = true,
  };

  mqtt = esp_mqtt_client_init(&mqtt_cfg);

  if (mqtt == NULL) {
    ERROR_SYSLOG(&init, MQTT, "client create failure", "MQTT_INIT_FAIL");
    goto finish;
  }

  esp_mqtt_client_register_event(mqtt, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

  if (esp_mqtt_client_start(mqtt) != ESP_OK) {
    ERROR_SYSLOG(&init, MQTT, "client start failure", "MQTT_START_FAIL");
    esp_mqtt_client_destroy(mqtt);
    mqtt = NULL;
    goto finish;
  }

  // create mqtt publisher task
  if (xTaskCreatePinnedToCore(mqtt_task, "mqtt", 4096, NULL, 5, NULL, CORE0) != pdPASS) {
    ERROR_SYSLOG(&init, MQTT, "task create failure", "MQTT_TASK_FAIL");
    esp_mqtt_client_stop(mqtt);
    esp_mqtt_client_destroy(mqtt);
    mqtt = NULL;
    goto finish;
  }

finish:
  if (IS_OK(&init, MQTT)) {
    CLEAR_ALL(&logbuf.run, MQTT);
    SYSLOG("MQTT_RDY");
  } else {
    COPY_STATE(&logbuf.run, &init, MQTT);
  }
}
