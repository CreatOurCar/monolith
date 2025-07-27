import mqtt from 'mqtt';
import { ref } from 'vue'
import ToastEventBus from 'primevue/toasteventbus';
import { connection_server, update_connection_server, update_connection_device } from './topbar';

let mqtt_client = null;

export function init_mqtt() {
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
}
