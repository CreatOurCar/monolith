import ToastEventBus from 'primevue/toasteventbus';

import dayjs from 'dayjs/esm';
import duration from 'dayjs/esm/plugin/duration';
import relativeTime from 'dayjs/esm/plugin/relativeTime';

dayjs.extend(duration);
dayjs.extend(relativeTime);

import mqtt from 'mqtt';

import { term } from '@/service/terminal';
import { update_telemetry } from '@/service/telemetry';
import { connection, config, times, files, format_size } from '@/service/state';
import { parse_cfg, parse_log, parse_logbuf, to_uint } from '@/service/protocol';
import { update_connection_server, update_connection_device } from '@/service/topbar';

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
    if (connection.server.value !== 'Disconnected') {
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
          times.boot.raw = null;
          update_connection_device(false);
          ToastEventBus.emit('add', { severity: 'error', summary: 'Device Offline', group: 'br', life: 5000 });
        } else {
          times.boot.raw = to_uint(32, message, 0);
          times.boot.value = dayjs(times.boot.raw * 1000).format("YYYY-MM-DD HH:mm:ss");
          update_connection_device(true);
        }
        break;
      }

      case 'd/cfg': {
        if (connection.device.value !== 'Online') {
          return;
        }

        const cfg = parse_cfg(message);
        config.net.ssid.value = cfg.wifi.ssid;
        config.net.passwd.value = cfg.wifi.passwd;
        config.dev.tz.value = cfg.device.tz;
        config.dev.intv.value = cfg.device.intv;
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
          term.write(`[${dayjs(times.boot.raw * 1000 + log.timestamp).format("HH:mm:ss.SSS")}] ${log.sys.msg}\n`);
        } catch (e) {
          console.error(e, message);
        }
        break;
      }

      case 'd': {
        const logbuf = parse_logbuf(message);
        times.current.value = dayjs(times.boot.raw * 1000 + logbuf.timestamp).format("YYYY-MM-DD HH:mm:ss.SSS");

        const d = dayjs.duration(logbuf.timestamp);
        const hours = Math.floor(d.asHours());
        const minutes = d.minutes();
        const seconds = d.seconds();

        times.uptime.value = '';

        if (hours > 0) times.uptime.value += `${hours} hr `;
        if (minutes > 0) times.uptime.value += `${minutes} min `;
        times.uptime.value += `${seconds} sec`;

        update_telemetry(logbuf);
        break;
      }

      case 'ack/set': {
        if (message.toString() === 'ok') {
          config.disabled = false;

          if (config.current_loading) {
            const [section, field] = config.current_loading.split("/");
            config[section][field].loading = false;
            config.current_loading = "";
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

      case 'ack/evt': {
        if (message.toString() === 'ok') {
          ToastEventBus.emit('add', { severity: 'success', summary: 'Event Saved', group: 'br', life: 3000 });
        }
        break;
      }

      case 'ack/ls': {
        if (!files.loading.list) {
          return;
        }

        files.loading.list = false;
        files.disabled = false;

        if (message.toString() === 'ok') {
          files.list = JSON.parse(JSON.stringify(files.buf.sort((a, b) => b.name.localeCompare(a.name))));

          ToastEventBus.emit('add', {
            severity: 'success',
            summary: 'File List Loaded',
            detail: `Found ${files.list.length} files.`,
            group: 'br',
            life: 3000
          });
        } else {
          ToastEventBus.emit('add', {
            severity: 'error',
            summary: 'File List Error',
            detail: message.toString(),
            group: 'br',
            life: 5000
          });
        }
        break;
      }

      case 'ack/del': {
        files.loading.del = false;
        files.disabled = false;

        if (message.toString() === 'ok') {
          ToastEventBus.emit('add', { severity: 'success', summary: 'File Deleted', group: 'br', life: 3000 });

          files.buf.length = 0;
          files.list.length = 0;
          setTimeout(() => {
            document.getElementById('list').click();
          }, 100);
        } else {
          ToastEventBus.emit('add', {
            severity: 'error',
            summary: 'File Delete Error',
            detail: message.toString(),
            group: 'br',
            life: 5000
          });
        }
        break;
      }

      case 'ack/get': {
        if (!files.loading.download) {
          return;
        }

        const cnt = to_uint(32, message, 0);

        for (let i = 0; i < cnt; i++) {
          if (!files.download.buf[i]) {
            ToastEventBus.emit('add', {
              severity: 'error',
              summary: 'File Download Error',
              detail: `Missing file chunk ${i}`,
              group: 'br',
              life: 5000
            });

            files.loading.download = false;
            files.disabled = false;
            return;
          }
        }

        const size = files.download.buf.reduce((acc, chunk) => acc + chunk.byteLength, 0);
        const file = new Uint8Array(size);

        let offset = 0;

        for (let i = 0; i < cnt; i++) {
          file.set(files.download.buf[i], offset);
          offset += files.download.buf[i].byteLength;
        }

        files.loading.download = false;
        files.disabled = false;

        ToastEventBus.emit('add', {
          severity: 'success', summary: 'File Downloaded',
          detail: `${files.download.name}\n(${format_size(size)}, ${files.download.speed})`,
          group: 'br', life: 3000
        });

        const blob = new Blob([file], { type: 'application/octet-stream' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = files.download.name;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        break;
      }

      default: {
        if (topic.startsWith('ack/ls/')) {
          const name = topic.replace('ack/ls/', '');

          if (!files.buf.some(file => file.name === name)) {
            files.buf.push({
              name: name,
              size: to_uint(32, message, 0),
            });
          }
        } else if (topic.startsWith('ack/get/')) {
          const index = topic.replace('ack/get/', '');
          files.download.buf[index] = message;
          files.download.time;
          files.download.speed = `${format_size(index * 4096 / ((new Date().getTime() - files.download.time) / 1000))}/s`;
          files.download.progress = (index * 4096 / files.download.size * 100).toFixed(1);
          break;
        }
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

