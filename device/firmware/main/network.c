#include "main.h"

char ssid[32];
char passwd[32];

void task_network(void *pvParameters) {
  nvs_handle_t nvs;

  if (nvs_open("network", NVS_READWRITE, &nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "nvs open failure", "NET_NVS_FAIL");
  }

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

  // no SSID or password set, init AP mode
  if (strlen(ssid) == 0 || strlen(passwd) == 0) {
    init_ap();
  }

  if (nvs_commit(nvs) != ESP_OK) {
    STATE_SYSLOG(STATE_ERR, "NETWORK", "nvs commit failure", "NVS_COMMIT_FAIL");
  }

  nvs_close(nvs);

  // start Wi-Fi connection

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
