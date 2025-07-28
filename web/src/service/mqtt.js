import mqtt from 'mqtt';
import { ref, watch } from 'vue'
import ToastEventBus from 'primevue/toasteventbus';

import { connection_server, update_connection_server, update_connection_device } from '@/service/topbar';
import { parse_cfg } from '@/service/protocol';
import { refs, disabled } from '@/service/state';

let mqtt_client = null;

export function init_mqtt() {
  mqtt_client = null;

  if (!localStorage.getItem('server/addr')) {
    localStorage.setItem('server/addr', 'monolith-v2.luftaquila.io');
  }

  mqtt_client = mqtt.connect({
    protocol: 'wss',
    host: localStorage.getItem('server/addr'),
    port: 443,
    username: localStorage.getItem('server/name') || '',
    password: localStorage.getItem('server/key') || '',
  });

  mqtt_client.on('connect', () => {
    update_connection_server(true);
    mqtt_client.subscribe(`${localStorage.getItem('server/name')}/d/#`);
  });

  mqtt_client.on('error', (e) => {
    ToastEventBus.emit('add', { severity: 'error', summary: 'Error', detail: `Server error: ${e}`, group: 'br', life: 3000 });
    update_connection_server(false);
  });

  mqtt_client.on('close', () => {
    if (connection_server.value.value !== 'Disconnected') {
      ToastEventBus.emit('add', { severity: 'error', summary: 'Error', detail: `Server connection closed`, group: 'br', life: 3000 });
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
        message = message.toString();

        if (message === 'OFFLINE') {
          update_connection_device(false);
        } else {
          update_connection_device(true);
          // TODO:
        }
        break;
      }

      case 'd/cfg': {
        const cfg = parse_cfg(message);
        refs.device.ssid.value = cfg.wifi.ssid;
        refs.device.password.value = cfg.wifi.passwd;
        refs.device.timezone.value = cfg.device.tz;
        refs.gps.enabled.value = cfg.en.gps ? true : false;
        refs.gps.dev.value = cfg.gps.dev;
        refs.can.enabled.value = cfg.en.can ? true : false;
        refs.can.bps.value = cfg.can.bps;
        refs.can.filter.value = '0x' + cfg.can.filter.toString(16).padStart(8, '0').toUpperCase();
        refs.can.mask.value = '0x' + cfg.can.mask.toString(16).padStart(8, '0').toUpperCase();
        refs.analog.enabled.value = cfg.en.analog ? true : false;
        refs.digital.enabled.value = cfg.en.digital ? true : false;
        break;
      }

      case 'd/sl': {
        break;
      }

      case 'd': {

        break;
      }
    }

    return;

    const device = topic.split('/')[2];
    const payload = JSON.parse(message.toString());

    if (payload && payload.type === 'device') {
      update_connection_device(device, true);
    }

    ToastEventBus.emit('add', { severity: 'info', summary: 'Message', detail: `Received message from ${device}: ${JSON.stringify(payload)}`, group: 'br', life: 3000 });
  });
}
