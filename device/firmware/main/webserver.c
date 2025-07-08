#include "cJSON.h"
#include "main.h"

const char *html =
  "<!DOCTYPE html> \
<html lang='en'> \
<head> \
  <meta charset='UTF-8'> \
  <meta name='viewport' content='width=device-width, initial-scale=1.0'> \
  <title>Monolith v2</title> \
  <style> \
    body { \
      display: flex; align-items: center; flex-direction: column; \
      height: 100vh; margin: 0; padding: 0; line-height: 1.3; \
      font-size: 1rem; font-weight: bold; text-align: center; \
    } \
    p { margin: 0.5rem 0; } \
    div { margin: 1rem 0 0.5rem; } \
    input { height: 1.8rem; width: 70vw; font-size: 1rem; text-align: center; } \
    button { font-size: 1.2rem; padding: 0.5rem 2rem; } \
    body, input, button { font-family: monospace, system-ui, sans-serif; } \
  </style> \
</head> \
<body> \
  <h3 style='font-size: 1.6rem; margin-top: 3rem;'>Monolith v2<br>Device Configuration</h3> \
  <p>ID: <span id='deviceid'>N/A</span></p> \
  <div><p>Wi-Fi SSID</p><input id='ssid' maxlength='30' placeholder=''></div> \
  <div><p>Wi-Fi Password</p><input id='passwd' maxlength='30' placeholder=''></div> \
  <div><p>Server Address</p><input id='server' maxlength='50' placeholder=''></div> \
  <div><p>Device Name</p><input id='name' maxlength='30' placeholder=''></div> \
  <div><p>Device Key</p><input id='key' maxlength='30' placeholder=''></div> \
  <div style='margin-top: 2.5rem;'><button id='save'>Save</button></div> \
    <script> \
    document.addEventListener('DOMContentLoaded', async () => { \
      try { \
        const res = await fetch('/config'); \
        if (!res.ok) throw new Error(`failed to load config: ${res.status}`); \
        const cfg = await res.json(); \
        document.getElementById('deviceid').textContent = cfg.id     || 'N/A'; \
        document.getElementById('ssid').value           = cfg.ssid   || ''; \
        document.getElementById('passwd').value         = cfg.passwd || ''; \
        document.getElementById('server').value         = cfg.server || ''; \
        document.getElementById('name').value           = cfg.name   || ''; \
        document.getElementById('key').value            = cfg.key    || ''; \
      } catch (e) { \
        alert(e.toString()); \
      } \
      document.getElementById('save').addEventListener('click', async () => { \
        const payload = { \
          ssid:   document.getElementById('ssid').value.trim(), \
          passwd: document.getElementById('passwd').value.trim(), \
          server: document.getElementById('server').value.trim(), \
          name:   document.getElementById('name').value.trim(), \
          key:    document.getElementById('key').value.trim() \
        }; \
        try { \
          const res = await fetch('/config', { \
            method: 'POST', \
            headers: { 'Content-Type': 'application/json' }, \
            body: JSON.stringify(payload) \
          }); \
          if (!res.ok) throw new Error(`failed to send config: ${res.status}`); \
          alert('Configuration saved. Please reboot the device.'); \
        } catch (e) { \
          alert(e); \
        } \
      }); \
    }); \
  </script> \
</body> \
</html>";

esp_err_t htmlprovider(httpd_req_t *req) {
  httpd_resp_send(req, html, strlen(html));
  return ESP_OK;
}

extern char ssid[32];
extern char passwd[32];
extern char server[64];
extern char name[32];
extern char key[32];
extern char macaddr[20];

static esp_err_t getconf(httpd_req_t *req) {
  nvs_handle_t nvs;

  if (nvs_open("network", NVS_READONLY, &nvs) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NET_NVS_FAIL");
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

  len = sizeof(key);
  nvs_get_str(nvs, "key", key, &len);

  nvs_close(nvs);

  char res[288];
  snprintf(res, sizeof(res), "{\"id\":\"%s\", \"ssid\":\"%s\", \"passwd\":\"%s\", \"server\":\"%s\", \"name\":\"%s\", \"key\":\"%s\"}",
    macaddr, ssid, passwd, server, name, key);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, res, strlen(res));
  return ESP_OK;
}

static esp_err_t setconf(httpd_req_t *req) {
  nvs_handle_t nvs;

  if (nvs_open("network", NVS_READWRITE, &nvs) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NET_NVS_FAIL");
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

  len = sizeof(key);
  nvs_get_str(nvs, "key", key, &len);

  nvs_close(nvs);

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"OK\"}");
  return ESP_OK;
}

httpd_handle_t webserver(void) {
  if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "NETIF init failure", "NETIF_INIT_FAIL");
  }

  esp_netif_create_default_wifi_ap();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  if (esp_wifi_init(&cfg) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "esp_wifi_init failed", "WIFI_INIT_FAIL");
  }

  wifi_config_t wifi = {
    .ap = {
      .ssid = WIFI_AP_SSID,
      .ssid_len = strlen(WIFI_AP_SSID),
      .password = WIFI_AP_PASSWD,
      .max_connection = 4,
      .authmode = WIFI_AUTH_WPA2_PSK,
    },
  };

  if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK || esp_wifi_set_config(WIFI_IF_AP, &wifi) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "AP config failure", "AP_CONFIG_FAIL");
  }

  if (esp_wifi_start() != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "AP start failure", "AP_START_FAIL");
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
    return server;
  }

  return NULL;
}
