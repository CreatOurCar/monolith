#include "main.h"

#include "driver/i2c_master.h"

/*******************************************************************************
 * Gyroscope monitor task
 ******************************************************************************/
void task_gyroscope(void *pvParameters) {
  i2c_master_bus_handle_t i2c0;

  // i2c0 already initalized by main thread
  if (i2c_master_get_bus_handle(I2C_NUM_0, &i2c0) != ESP_OK) {
    ERROR_SYSLOG(&init, GYRO, "I2C get bus failure", "GYR_INIT_FAIL");
    vTaskDelete(NULL);
  }

  i2c_device_config_t gyro_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = 0x68,
    .scl_speed_hz    = 100000,
  };

  i2c_master_dev_handle_t gyro;

  if (i2c_master_bus_add_device(i2c0, &gyro_cfg, &gyro) != ESP_OK) {
    ERROR_SYSLOG(&init, GYRO, "device init failure", "GYR_DEV_FAIL");
    vTaskDelete(NULL);
  }

  uint8_t tx[7] = { 0x1B, 1 << 3, 1 << 4 };  // GYRO_CONFIG register address, 500dps and 8g full scale

  if (i2c_master_transmit(gyro, tx, 3, 10) != ESP_OK) {
    ERROR_SYSLOG(&init, GYRO, "gyro init failure", "GYR_CFG_FAIL");
  }

  const int sample = 100;
  int valid = 0;

  int32_t sum_x = 0;
  int32_t sum_y = 0;
  int32_t sum_z = 0;

  tx[0] = 0x43;  // GYRO_XOUT_H register address

  for (int i = 0; i < sample; i++) {
    uint8_t rx[6] = { 0 };

    if (i2c_master_transmit_receive(gyro, tx, 1, rx, sizeof(rx), 10) == ESP_OK) {
      int16_t gyro_x = (int16_t)(((uint16_t)rx[0] << 8) | rx[1]);
      int16_t gyro_y = (int16_t)(((uint16_t)rx[2] << 8) | rx[3]);
      int16_t gyro_z = (int16_t)(((uint16_t)rx[4] << 8) | rx[5]);

      sum_x += gyro_x;
      sum_y += gyro_y;
      sum_z += gyro_z;
      valid++;
    }
  }

  int32_t off_x = -sum_x / valid / 2;
  int32_t off_y = -sum_y / valid / 2;
  int32_t off_z = -sum_z / valid / 2;

  tx[0] = 0x13;  // XG_OFFSET_H register address
  tx[1] = (uint8_t)(off_x >> 8);
  tx[2] = (uint8_t)(off_x & 0xFF);
  tx[3] = (uint8_t)(off_y >> 8);
  tx[4] = (uint8_t)(off_y & 0xFF);
  tx[5] = (uint8_t)(off_z >> 8);
  tx[6] = (uint8_t)(off_z & 0xFF);

  if (i2c_master_transmit(gyro, tx, 7, 10) != ESP_OK) {
    ERROR_SYSLOG(&init, GYRO, "gyro offset write failure", "GYR_OFS_FAIL");
  }

  if (IS_OK(&init, GYRO)) {
    CLEAR_ALL(&logbuf.run, GYRO);
    SYSLOG("GYR_RDY");
  } else {
    COPY_STATE(&logbuf.run, &init, GYRO);
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  tx[0] = 0x3B;  // ACCEL_XOUT_H register address

  while (true) {
    uint8_t rx[14] = { 0 };  // 0x3B ACCEL_XOUT_H to 0x48 GYRO_ZOUT_L register

    if (i2c_master_transmit_receive(gyro, tx, 1, rx, sizeof(rx), 10) == ESP_OK) {
      logbuf.gyro.payload.gyroscope.accel_x     = (int16_t)(((uint16_t)rx[0] << 8) | rx[1]);
      logbuf.gyro.payload.gyroscope.accel_y     = (int16_t)(((uint16_t)rx[2] << 8) | rx[3]);
      logbuf.gyro.payload.gyroscope.accel_z     = (int16_t)(((uint16_t)rx[4] << 8) | rx[5]);
      logbuf.gyro.payload.gyroscope.temperature = (int16_t)(((uint16_t)rx[6] << 8) | rx[7]);
      logbuf.gyro.payload.gyroscope.gyro_x      = (int16_t)(((uint16_t)rx[8] << 8) | rx[9]);
      logbuf.gyro.payload.gyroscope.gyro_y      = (int16_t)(((uint16_t)rx[10] << 8) | rx[11]);
      logbuf.gyro.payload.gyroscope.gyro_z      = (int16_t)(((uint16_t)rx[12] << 8) | rx[13]);
      LOG(LOG_TYPE_GYROSCOPE, &logbuf.gyro);
    } else {
      ERROR_SYSLOG(&logbuf.run, GYRO, "read transfer failure", "GYR_READ_FAIL");
    }

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}
