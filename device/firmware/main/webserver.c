#include "main.h"

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static esp_err_t htmlprovider(httpd_req_t *req) {
  httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
  return ESP_OK;
}

static esp_err_t getconf(httpd_req_t *req) {
  nvs_handle_t nvs;

  if (nvs_open("network", NVS_READONLY, &nvs) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "WEB_NVS_O_FAIL");
    ERROR_SYSLOG(NVS, "open failure: network", "WEB_NVS_O_FAIL");
    nvs_close(nvs);
    return ESP_FAIL;
  }

  size_t len = sizeof(ssid);
  nvs_get_str(nvs, "ssid", ssid, &len);

  len = sizeof(passwd);
  nvs_get_str(nvs, "passwd", passwd, &len);

  len = sizeof(server);
  nvs_get_str(nvs, "server", server, &len);

  len = sizeof(name);
  nvs_get_str(nvs, "name", name, &len);
  name_len = strlen(name);

  len = sizeof(key);
  nvs_get_str(nvs, "key", key, &len);

  nvs_close(nvs);

  char res[288];
  snprintf(res, sizeof(res),
    "{\"id\":\"%02X:%02X:%02X:%02X:%02X:%02X\", "
    "\"ssid\":\"%s\", \"passwd\":\"%s\", \"server\":\"%s\", \"name\":\"%s\", \"key\":\"%s\"}",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid, passwd, server, name, key);

  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, res, strlen(res));
  return ESP_OK;
}

static esp_err_t setconf(httpd_req_t *req) {
  nvs_handle_t nvs;

  if (nvs_open("network", NVS_READWRITE, &nvs) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "WEB_NVS_O_FAIL");
    ERROR_SYSLOG(NVS, "open failure: network", "WEB_NVS_O_FAIL");
    return ESP_FAIL;
  }

  char buf[256];
  size_t len = httpd_req_recv(req, buf, sizeof(buf) - 1);

  if (len <= 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "REQ_RECV_FAIL");
    nvs_close(nvs);
    return ESP_FAIL;
  }

  buf[len]    = '\0';
  cJSON *json = cJSON_Parse(buf);

  if (!json) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "INVALID_JSON");
    nvs_close(nvs);
    return ESP_FAIL;
  }

#define WRITE_STR_FIELD(field)                                      \
  do {                                                              \
    cJSON *item = cJSON_GetObjectItem(json, #field);                \
    if (cJSON_IsString(item) && item->valuestring) {                \
      nvs_set_str(nvs, #field, item->valuestring);                  \
    } else {                                                        \
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "BAD_FIELD"); \
      cJSON_Delete(json);                                           \
      nvs_close(nvs);                                               \
      return ESP_FAIL;                                              \
    }                                                               \
  } while (FALSE)

  WRITE_STR_FIELD(ssid);
  WRITE_STR_FIELD(passwd);
  WRITE_STR_FIELD(server);
  WRITE_STR_FIELD(name);
  WRITE_STR_FIELD(key);

#undef WRITE_STR_FIELD

  if (nvs_commit(nvs) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NVS_COMMIT_FAIL");
    ERROR_SYSLOG(NVS, "commit failure: network", "WEB_NVS_C_FAIL");
    nvs_close(nvs);
    return ESP_FAIL;
  }

  cJSON_Delete(json);

  len = sizeof(ssid);
  nvs_get_str(nvs, "ssid", ssid, &len);

  len = sizeof(passwd);
  nvs_get_str(nvs, "passwd", passwd, &len);

  len = sizeof(server);
  nvs_get_str(nvs, "server", server, &len);

  len = sizeof(name);
  nvs_get_str(nvs, "name", name, &len);
  name_len = strlen(name);

  len = sizeof(key);
  nvs_get_str(nvs, "key", key, &len);

  nvs_close(nvs);

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"OK\"}");
  return ESP_OK;
}

httpd_handle_t webserver(void) {
  if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK) {
    ERROR_SYSLOG(WIFI, "netif init failure", "NETIF_INIT_FAIL");
  }

  esp_netif_create_default_wifi_ap();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  if (esp_wifi_init(&cfg) != ESP_OK) {
    ERROR_SYSLOG(WIFI, "init failure", "WIFI_INIT_FAIL");
  }

  wifi_config_t wifi = {
    .ap = {
      .password = "monolith",
      .max_connection = 4,
      .authmode = WIFI_AUTH_WPA2_PSK,
    },
  };

  sprintf((char *)wifi.ap.ssid, "Monolith v2 %02X%02X%02X", mac[3], mac[4], mac[5]);

  if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK || esp_wifi_set_config(WIFI_IF_AP, &wifi) != ESP_OK) {
    ERROR_SYSLOG(WIFI, "AP config failure", "AP_CFG_FAIL");
  }

  if (esp_wifi_start() != ESP_OK) {
    ERROR_SYSLOG(WIFI, "AP start failure", "AP_START_FAIL");
  }

  httpd_handle_t server   = NULL;
  httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
  config.server_port      = 80;
  config.lru_purge_enable = true;

  httpd_uri_t root      = { .uri = "/", .method = HTTP_GET, .handler = htmlprovider, .user_ctx = NULL };
  httpd_uri_t getconfig = { .uri = "/config", .method = HTTP_GET, .handler = getconf, .user_ctx = NULL };
  httpd_uri_t setconfig = { .uri = "/config", .method = HTTP_POST, .handler = setconf, .user_ctx = NULL };

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &getconfig);
    httpd_register_uri_handler(server, &setconfig);
    SYSLOG("WEB_SVR_START");
    return server;
  }

  return NULL;
}
