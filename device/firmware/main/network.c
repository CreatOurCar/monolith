#include "main.h"

char ssid[32];
char passwd[32];
char server[64];
char name[32];
char key[32];
uint8_t mac[6];

#define WIFI_FAIL_BIT (1 << 0)
#define WIFI_CONNECTED_BIT (1 << 1)

static EventGroupHandle_t wifi_evtgrp;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    char buf[16];
    snprintf(buf, sizeof(buf), "STA_LOST:%02X", ((wifi_event_sta_disconnected_t*)event_data)->reason);
    STATE_SYSLOG(STATE_ERR, "NETWORK", "Wi-Fi disconnected", buf);
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(wifi_evtgrp, WIFI_CONNECTED_BIT);
  }
}

void task_network(void* pvParameters) {
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

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
  wifi_evtgrp = xEventGroupCreate();

  if (wifi_evtgrp == NULL) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "Event group creation failure", "WIFI_EVTGRP_FAIL");
  }

  if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "NETIF init failure", "NETIF_INIT_FAIL");
  }

  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();

  if (esp_wifi_init(&wifi_cfg) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "esp_wifi_init failed", "WIFI_INIT_FAIL");
  }

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

  wifi_config_t wifi = { 0 };
  snprintf((char*)wifi.sta.ssid, sizeof(wifi.sta.ssid), "%s", ssid);
  snprintf((char*)wifi.sta.password, sizeof(wifi.sta.password), "%s", passwd);
  wifi.sta.scan_method        = WIFI_ALL_CHANNEL_SCAN;
  wifi.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK || esp_wifi_set_config(WIFI_IF_STA, &wifi) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "STA config failure", "STA_CONFIG_FAIL");
  }

  if (esp_wifi_start() != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "STA start failure", "STA_START_FAIL");
  }

  EventBits_t bits = xEventGroupWaitBits(wifi_evtgrp, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, FALSE, FALSE, portMAX_DELAY);

  if (!(bits & WIFI_CONNECTED_BIT)) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "Wi-Fi connection failed", "STA_CONN_FAIL");
  }

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
