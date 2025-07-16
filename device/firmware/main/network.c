#include <time.h>

#include "main.h"

#include "driver/i2c_master.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"

char ssid[32];
char passwd[32];
char server[64];
char name[32];
char key[32];
uint8_t mac[6];
uint32_t name_len;

#define WIFI_FAIL_BIT (1 << 0)
#define WIFI_CONNECTED_BIT (1 << 1)

void mqtt_client(void);
httpd_handle_t webserver(void);

static EventGroupHandle_t wifi_evtgrp;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    char buf[16];
    snprintf(buf, sizeof(buf), "STA_LOST:%02X", ((wifi_event_sta_disconnected_t *)event_data)->reason);
    ERROR_SYSLOG(WIFI, buf, buf);
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(wifi_evtgrp, WIFI_CONNECTED_BIT);
    CLEAR_ERROR(WIFI);
    SYSLOG("WIFI_CONN");
    INFO(WIFI, "connected to %s (" IPSTR ")", ssid, IP2STR(&((ip_event_got_ip_t *)event_data)->ip_info.ip));
  }
}

static void sntp_sync_callback(struct timeval *tv) {
  i2c_master_bus_handle_t i2c0_handle;

  // i2c0 already initalized
  if (i2c_master_get_bus_handle(I2C_NUM_0, &i2c0_handle) != ESP_OK) {
    ERROR_SYSLOG(RTC, "I2C get bus failure", "RTC_I2C_FAIL");
  }

  i2c_master_dev_handle_t rtc_handle;
  i2c_device_config_t rtc_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x51,
    .scl_speed_hz    = 100000,
  };

  if (i2c_master_bus_add_device(i2c0_handle, &rtc_cfg, &rtc_handle) != ESP_OK) {
    ERROR_SYSLOG(RTC, "device init failure", "RTC_DEV_FAIL");
  }

  struct tm tp;
  struct tm *tm = gmtime_r(&tv->tv_sec, &tp);

  uint8_t tx[8];
  tx[0] = 0x02;                           // VL_seconds register address
  tx[1] = DEC_TO_BCD(tm->tm_sec);         // seconds
  tx[2] = DEC_TO_BCD(tm->tm_min);         // minutes
  tx[3] = DEC_TO_BCD(tm->tm_hour);        // hours
  tx[4] = DEC_TO_BCD(tm->tm_wday);        // day of week
  tx[5] = DEC_TO_BCD(tm->tm_mday);        // day of month
  tx[6] = DEC_TO_BCD(tm->tm_mon + 1);     // month
  tx[7] = DEC_TO_BCD(tm->tm_year - 100);  // year

  if (i2c_master_transmit(rtc_handle, tx, sizeof(tx), 100) != ESP_OK) {
    ERROR_SYSLOG(RTC, "write time transfer failure", "RTC_WRITE_FAIL");
  }

  i2c_master_bus_rm_device(rtc_handle);
  SYSLOG("SNTP_SYNC");
  INFO(RTC, "SNTP time set to %s", ctime(&tv->tv_sec));
}

void task_network(void *pvParameters) {
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  size_t len = sizeof(ssid);

  if (nvs_get_str(nvs, "ssid", ssid, &len) != ESP_OK) {
    ssid[0] = '\0';
    nvs_set_str(nvs, "ssid", "");
  }

  len = sizeof(passwd);

  if (nvs_get_str(nvs, "passwd", passwd, &len) != ESP_OK) {
    passwd[0] = '\0';
    nvs_set_str(nvs, "passwd", "");
  }

  len = sizeof(server);

  if (nvs_get_str(nvs, "server", server, &len) != ESP_OK) {
    snprintf(server, sizeof(server), "v2.monolith.luftaquila.io");
    nvs_set_str(nvs, "server", server);
  }

  len = sizeof(name);

  if (nvs_get_str(nvs, "name", name, &len) != ESP_OK) {
    snprintf(name, sizeof(name), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    nvs_set_str(nvs, "name", name);
  }

  name_len = strlen(name);

  len = sizeof(key);

  if (nvs_get_str(nvs, "key", key, &len) != ESP_OK) {
    snprintf(key, sizeof(key), "monolith");
    nvs_set_str(nvs, "key", key);
  }

  if (nvs_commit(nvs) != ESP_OK) {
    ERROR_SYSLOG(NVS, "commit failure: network", "NET_NVS_FAIL");
  }

  // no SSID or proper password set, init AP mode
  if (strlen(ssid) == 0 || strlen(passwd) < 8) {
    if (webserver() == NULL) {
      ERROR_SYSLOG(WIFI, "HTTP server init failure", "WEBSERVER_FAIL");
    }

    vTaskDelete(NULL);
  }

  // start Wi-Fi connection
  wifi_evtgrp = xEventGroupCreate();

  if (wifi_evtgrp == NULL) {
    ERROR_SYSLOG(WIFI, "event group creation failure", "WIFI_EVTGRP_FAIL");
  }

  if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK) {
    ERROR_SYSLOG(WIFI, "netif init failure", "NETIF_INIT_FAIL");
  }

  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();

  if (esp_wifi_init(&wifi_cfg) != ESP_OK) {
    ERROR_SYSLOG(WIFI, "init failure", "WIFI_INIT_FAIL");
  }

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

  wifi_config_t wifi = { 0 };
  snprintf((char *)wifi.sta.ssid, sizeof(wifi.sta.ssid), "%s", ssid);
  snprintf((char *)wifi.sta.password, sizeof(wifi.sta.password), "%s", passwd);
  wifi.sta.scan_method        = WIFI_ALL_CHANNEL_SCAN;
  wifi.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK || esp_wifi_set_config(WIFI_IF_STA, &wifi) != ESP_OK) {
    ERROR_SYSLOG(WIFI, "STA config failure", "STA_CFG_FAIL");
  }

  if (esp_wifi_start() != ESP_OK) {
    ERROR_SYSLOG(WIFI, "STA start failure", "STA_START_FAIL");
  }

  EventBits_t bits = xEventGroupWaitBits(wifi_evtgrp, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, FALSE, FALSE, portMAX_DELAY);

  if (!(bits & WIFI_CONNECTED_BIT)) {
    ERROR_SYSLOG(WIFI, "connection failed", "STA_CONN_FAIL");
  }

  // SNTP time sync service
  esp_sntp_config_t sntp = ESP_NETIF_SNTP_DEFAULT_CONFIG("time.google.com");
  sntp.sync_cb           = sntp_sync_callback;
  esp_netif_sntp_init(&sntp);

  mqtt_client();

  vTaskDelete(NULL);
}
