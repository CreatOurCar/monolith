#include "main.h"

char ssid[32];
char passwd[32];

void init_ap(nvs_handle_t nvs_handle);

void task_network(void *pvParameters) {
  esp_err_t err;
  nvs_handle_t nvs_handle;

  if ((err = nvs_open("storage", NVS_READWRITE, &nvs_handle)) != ESP_OK) {
    ESP_LOGE("network", "NVS open failed (%s)", esp_err_to_name(err));
    return;
  }

  if ((err = nvs_get_str(nvs_handle, "ssid", ssid, NULL)) != ESP_OK) {
    init_ap(nvs_handle);
    return;
  }

  if ((err = nvs_get_str(nvs_handle, "passwd", passwd, NULL)) != ESP_OK) {
    init_ap(nvs_handle);
    return;
  }

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));  // Infinite loop to halt execution
  }
  // TODO

  // 3. 문자열 저장
  const char *my_str = "Hello ESP32!";
  err = nvs_set_str(nvs_handle, "my_str_key", my_str);
  if (err == ESP_OK) {
    ESP_LOGI("network", "String stored: %s", my_str);
  } else {
    ESP_LOGE("network", "Failed to store string");
  }

  // 4. 커밋 (실제 저장)
  err = nvs_commit(nvs_handle);
  ESP_ERROR_CHECK(err);

  nvs_close(nvs_handle);

  while (1) {
    ESP_LOGI("network", "hello world");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void init_ap(nvs_handle_t nvs_handle) {
  esp_err_t err;
  const char *my_str = "Hello ESP32!";

  err = nvs_set_str(nvs_handle, "my_str_key", my_str);
  if (err == ESP_OK) {
    ESP_LOGI("network", "String stored: %s", my_str);
  } else {
    ESP_LOGE("network", "Failed to store string");
  }

  // 4. 커밋 (실제 저장)
  err = nvs_commit(nvs_handle);
  ESP_ERROR_CHECK(err);

  nvs_close(nvs_handle);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));  // Infinite loop to halt execution
  }
}
