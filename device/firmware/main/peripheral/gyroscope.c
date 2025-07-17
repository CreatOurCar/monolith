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

  uint8_t tx[3] = { 0x1B, 1 << 3, 1 << 4 };  // GYRO_CONFIG register address, 500dps and 8g full scale

  if (i2c_master_transmit(gyro, tx, sizeof(tx), 10) != ESP_OK) {
    ERROR_SYSLOG(&init, GYRO, "gyro init failure", "GYR_CFG_FAIL");
  }

  if (IS_OK(&init, GYRO)) {
    CLEAR_ALL(&run, GYRO);
    SYSLOG("GYR_RDY");
  } else {
    COPY_STATE(&run, &init, GYRO);
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  tx[0] = 0x3B;  // ACCEL_XOUT_H register address

  while (TRUE) {
    uint8_t rx[14] = { 0 };  // 0x3B ACCEL_XOUT_H to 0x48 GYRO_ZOUT_L register

    if (i2c_master_transmit_receive(gyro, tx, 1, rx, sizeof(rx), 10) != ESP_OK) {
      ERROR_SYSLOG(&run, GYRO, "read transfer failure", "GYR_READ_FAIL");
      continue;
    }

    log_t log;
    log.payload.gyroscope.accel_x     = (int16_t)(((uint16_t)rx[1] << 8) | rx[1]);
    log.payload.gyroscope.accel_y     = (int16_t)(((uint16_t)rx[2] << 8) | rx[3]);
    log.payload.gyroscope.accel_z     = (int16_t)(((uint16_t)rx[4] << 8) | rx[5]);
    log.payload.gyroscope.temperature = (int16_t)(((uint16_t)rx[6] << 8) | rx[7]);
    log.payload.gyroscope.gyro_x      = (int16_t)(((uint16_t)rx[8] << 8) | rx[9]);
    log.payload.gyroscope.gyro_y      = (int16_t)(((uint16_t)rx[10] << 8) | rx[11]);
    log.payload.gyroscope.gyro_z      = (int16_t)(((uint16_t)rx[12] << 8) | rx[13]);
    LOG(LOG_TYPE_GYROSCOPE, &log);

    INFO(GYRO, "Ax: %d Ay: %d Az: %d Gx: %d Gy: %d Gz: %d T: %d", log.payload.gyroscope.accel_x,
      log.payload.gyroscope.accel_y, log.payload.gyroscope.accel_z, log.payload.gyroscope.gyro_x,
      log.payload.gyroscope.gyro_y, log.payload.gyroscope.gyro_z, log.payload.gyroscope.temperature);

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}
