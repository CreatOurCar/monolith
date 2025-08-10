#include "main.h"

#include "driver/twai.h"

#define CAN_ALERT_ERROR                                                                                        \
  (TWAI_ALERT_ERR_ACTIVE | TWAI_ALERT_RECOVERY_IN_PROGRESS | TWAI_ALERT_ARB_LOST | TWAI_ALERT_ABOVE_ERR_WARN | \
    TWAI_ALERT_BUS_ERROR | TWAI_ALERT_TX_FAILED | TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_ERR_PASS |             \
    TWAI_ALERT_BUS_OFF | TWAI_ALERT_RX_FIFO_OVERRUN)

#define CAN_ALERT_ENABLED (CAN_ALERT_ERROR)

static inline twai_timing_config_t select_baud(uint8_t can_bps) {
  switch (can_bps) {
    case CAN_BPS_1K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_1KBITS();
    case CAN_BPS_5K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_5KBITS();
    case CAN_BPS_10K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_10KBITS();
    case CAN_BPS_12_5K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_12_5KBITS();
    case CAN_BPS_16K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_16KBITS();
    case CAN_BPS_20K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_20KBITS();
    case CAN_BPS_25K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_25KBITS();
    case CAN_BPS_50K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_50KBITS();
    case CAN_BPS_100K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_100KBITS();
    case CAN_BPS_125K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_125KBITS();
    case CAN_BPS_250K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
    case CAN_BPS_500K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
    case CAN_BPS_800K:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_800KBITS();
    case CAN_BPS_1M:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();
    default:
      return (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
  }
}

/*******************************************************************************
 * CAN traffic monitor / transmitter task
 ******************************************************************************/
void task_can(void *pvParameters) {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_7, GPIO_NUM_6, TWAI_MODE_NORMAL);
  g_config.alerts_enabled        = CAN_ALERT_ENABLED;
  twai_timing_config_t t_config  = select_baud(storage.can.bps);
  twai_filter_config_t f_config  = {
     .single_filter   = true,
     .acceptance_code = storage.can.filter,
     .acceptance_mask = storage.can.mask,
  };

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK || twai_start() != ESP_OK) {
    ERROR_SYSLOG(&init, CAN, "driver init failure", "CAN_INIT_FAIL");
  }

  if (IS_OK(&init, CAN)) {
    CLEAR_ALL(&logbuf.run, CAN);
    SYSLOG("CAN_RDY");
  } else {
    COPY_STATE(&logbuf.run, &init, CAN);
  }

  uint32_t prev_alerts = 0;

  while (true) {
    esp_err_t ret;
    twai_message_t msg;

    do {
      if ((ret = xQueueReceive(cantxqueue, &msg, 0)) == true) {
        if (twai_transmit(&msg, pdMS_TO_TICKS(1)) != ESP_OK) {
          ERROR_SYSLOG(&logbuf.run, CAN, "transmit failure", "CAN_TX_FAIL");
        } else {
          char buf[sizeof(system_event_t)];
          snprintf(buf, sizeof(buf), "CANTX:%lX", msg.identifier);
          SYSLOG(buf);
        }
      }
    } while (ret);

    if (twai_receive(&msg, pdMS_TO_TICKS(120)) != ESP_OK) {
      continue;
    }

    log_t log;
    log.payload.can.id       = msg.identifier;
    log.payload.can.extended = msg.extd;
    log.payload.can.remote   = msg.rtr;
    log.payload.can.len      = msg.data_length_code;
    memcpy(log.payload.can.data, msg.data, msg.data_length_code);
    LOG(LOG_TYPE_CAN, &log);
    xQueueSend(canlogqueue, &log, 0);

    uint32_t alerts = 0;
    twai_read_alerts(&alerts, 0);

    if (alerts) {
      if (alerts == prev_alerts) {
        continue;
      }

      char buf[sizeof(system_event_t)];
      snprintf(buf, sizeof(buf), "CANALT:%lX", alerts);
      SYSLOG(buf);

      if (alerts & CAN_ALERT_ERROR) {
        SET_ERROR(&logbuf.run, CAN);
        prev_alerts = alerts;
      } else {
        CLEAR_ERROR(&logbuf.run, CAN);
      }
    } else if (prev_alerts) {
      CLEAR_ERROR(&logbuf.run, CAN);
    }
  }
}
