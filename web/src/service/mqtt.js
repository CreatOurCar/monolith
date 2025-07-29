import ToastEventBus from 'primevue/toasteventbus';

import 'dayjs';
import mqtt from 'mqtt';

import { term } from '@/service/terminal';
import { update_telemetry } from '@/service/telemetry';
import { config } from '@/service/state';
import { parse_cfg, parse_log, parse_logbuf, to_uint } from '@/service/protocol';
import { connection_server, connection_device, update_connection_server, update_connection_device } from '@/service/topbar';

let boot = null;
let mqtt_client = null;
let first_auth_fail = true;

export function init_mqtt() {
  if (mqtt_client) {
    mqtt_client.end();
    mqtt_client = null;
  }

  if (!localStorage.getItem('server/addr')) {
    localStorage.setItem('server/addr', 'monolith-v2.luftaquila.io');
  }

  mqtt_client = mqtt.connect({
    protocol: 'wss',
    host: localStorage.getItem('server/addr'),
    port: 443,
    username: localStorage.getItem('server/name') || '',
    password: localStorage.getItem('server/key') || '',
    keepalive: 10,
    reschedulePings: false,
  });

  mqtt_client.on('connect', () => {
    update_connection_server(true);
    mqtt_client.subscribe(`${localStorage.getItem('server/name')}/d/#`);
    mqtt_client.subscribe(`${localStorage.getItem('server/name')}/ack/#`);
  });

  mqtt_client.on('error', (e) => {
    update_connection_server(false);

    if (e.message.includes('Not authorized')) {
      if (first_auth_fail) {
        first_auth_fail = false;
        ToastEventBus.emit('add', { severity: 'error', summary: 'Authentication Failed', group: 'br', life: 5000 });
      }
    } else {
      ToastEventBus.emit('add', { severity: 'error', summary: 'Server Error', detail: e, group: 'br', life: 5000 });
    }
  });

  mqtt_client.on('close', () => {
    if (connection_server.value.value !== 'Disconnected') {
      ToastEventBus.emit('add', { severity: 'error', summary: 'Server Connection Closed', group: 'br', life: 5000 });
    }
    update_connection_server(false);
  });

  mqtt_client.on('message', (topic, message) => {
    topic = topic.split('/');

    if (topic[0] !== localStorage.getItem('server/name')) {
      return;
    }

    topic = topic.slice(1).join('/');

    switch (topic) {
      case 'd/boot': {
        if (message.toString() === 'OFFLINE') {
          boot = null;
          update_connection_device(false);
          ToastEventBus.emit('add', { severity: 'error', summary: 'Device Offline', group: 'br', life: 5000 });
        } else {
          boot = to_uint(32, message, 0);
          update_connection_device(true);
          // TODO:
        }
        break;
      }

      case 'd/cfg': {
        if (connection_device.value.value !== 'Online') {
          return;
        }

        const cfg = parse_cfg(message);
        config.net.ssid.value = cfg.wifi.ssid;
        config.net.passwd.value = cfg.wifi.passwd;
        config.dev.tz.value = cfg.device.tz;
        config.gps.en.value = cfg.en.gps ? true : false;
        config.gps.dev.value = cfg.gps.dev;
        config.can.en.value = cfg.en.can ? true : false;
        config.can.bps.value = cfg.can.bps;
        config.can.filter.value = '0x' + cfg.can.filter.toString(16).padStart(8, '0').toUpperCase();
        config.can.mask.value = '0x' + cfg.can.mask.toString(16).padStart(8, '0').toUpperCase();
        config.anl.en.value = cfg.en.analog ? true : false;
        config.dgt.en.value = cfg.en.digital ? true : false;

        ToastEventBus.emit('add', { severity: 'success', summary: 'Configuration Loaded', group: 'br', life: 3000 });
        config.disabled = false;
        break;
      }

      case 'd/sl': {
        try {
          const log = parse_log(message);
          term.write(`[SYS ${dayjs(boot * 1000 + log.timestamp).format("HH:mm:ss.SSS")}] ${log.sys.msg}\n`);
        } catch (e) {
          console.error(e);
          console.warn(message);
        }
        break;
      }

      case 'd': {
        update_telemetry(parse_logbuf(message));
        break;
      }

      case 'ack/set': {
        if (message.toString() === 'ok') {
          config.disabled.value = false;

          if (config.current_loading.value) {
            const [section, field] = config.current_loading.value.split("/");
            config[section][field].loading = false;
            config.current_loading.value = "";
          }

          ToastEventBus.emit('add', {
            severity: 'success',
            summary: 'Configuration Saved',
            detail: `Restart the device to apply changes.`,
            group: 'br',
            life: 3000
          });
        }
        break;
      }
    }
  });
}

export function publish(topic, payload, qos) {
  if (!mqtt_client || !mqtt_client.connected) {
    ToastEventBus.emit('add', { severity: 'error', summary: 'Server Disconnected', group: 'br', life: 5000 });
    return;
  }

  mqtt_client.publish(`${localStorage.getItem('server/name')}/${topic}`, payload, { qos: qos });
}

