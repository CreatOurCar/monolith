#include "main.h"

char ssid[32];
char passwd[32];
char server[64];
char name[32];
char key[32];
char macaddr[20];

void task_network(void *pvParameters) {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(macaddr, sizeof(macaddr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  nvs_handle_t nvs;

  // read and save default values
  if (nvs_open("network", NVS_READWRITE, &nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "nvs open failure", "NET_NVS_FAIL");
  }

  size_t len = sizeof(ssid);

  if (nvs_get_str(nvs, "ssid", ssid, &len) != ESP_OK) {
    nvs_set_str(nvs, "ssid", "");
    len = sizeof(ssid);
    nvs_get_str(nvs, "ssid", ssid, &len);
  }

  len = sizeof(passwd);

  if (nvs_get_str(nvs, "passwd", passwd, &len) != ESP_OK) {
    nvs_set_str(nvs, "passwd", "");
    len = sizeof(passwd);
    nvs_get_str(nvs, "passwd", passwd, &len);
  }

  len = sizeof(server);

  if (nvs_get_str(nvs, "server", server, &len) != ESP_OK) {
    nvs_set_str(nvs, "server", DEFAULT_SERVER);
    len = sizeof(server);
    nvs_get_str(nvs, "server", server, &len);
  }

  len = sizeof(name);

  if (nvs_get_str(nvs, "name", name, &len) != ESP_OK) {
    snprintf(name, sizeof(name), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    nvs_set_str(nvs, "name", name);
    len = sizeof(name);
    nvs_get_str(nvs, "name", name, &len);
  }

  len = sizeof(key);

  if (nvs_get_str(nvs, "key", key, &len) != ESP_OK) {
    nvs_set_str(nvs, "key", "monolith");
    len = sizeof(key);
    nvs_get_str(nvs, "key", key, &len);
  }

  if (nvs_commit(nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "nvs commit failure", "NVS_COMMIT_FAIL");
  }

  nvs_close(nvs);

  // no SSID or proper password set, init AP mode
  if (strlen(ssid) == 0 || strlen(passwd) < 8) {
    if (webserver() == NULL) {
      STATE_SYSLOG(STATE_ERR, "NETWORK", "HTTP server start failure", "WEBSERVER_FAIL");
    }

    // won't proceed further on AP mode
    while (TRUE) {
      vTaskDelay(pdMS_TO_TICKS(10000));
    }
  }

  // start Wi-Fi connection

  ESP_LOGE("NETWORK", "waiting!");

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
