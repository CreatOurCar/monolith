import { ref } from 'vue'
import ToastEventBus from 'primevue/toasteventbus';

/*******************************************************************************
 * Topbar connection status
 *******************************************************************************/
export const connection_server = ref({
  value: 'Uninitialized',
  severity: 'danger',
});

export const connection_device = ref({
  value: 'Offline',
  severity: 'danger',
});

export const connection_status = ref('text-gray-500');

export const update_connection_server = (value) => {
  if (value === false) {
    connection_server.value.value = 'Disconnected';
    connection_server.value.severity = 'danger';
    connection_status.value = 'text-gray-500';
  } else {
    connection_server.value.value = 'Connected';
    connection_server.value.severity = 'success';
    connection_status.value = connection_device.value.value === 'Online' ? 'text-green-500' : 'text-red-400';
    ToastEventBus.emit('add', {severity: 'success', summary: 'Server Connected', detail: `Connected to server`, group: 'br', life: 3000});
  }
}

export const update_connection_device = (value) => {
  if (value === false) {
    connection_device.value.value = 'Offline';
    connection_device.value.severity = 'danger';

    if (connection_server.value.value === 'Connected') {
      connection_status.value = 'text-red-400';
    }
  } else {
    connection_device.value.value = 'Online';
    connection_device.value.severity = 'success';

    if (connection_server.value.value === 'Connected') {
      connection_status.value = 'text-green-500';
      ToastEventBus.emit('add', {severity: 'success', summary: 'Device Online', detail: `Device is now online`, group: 'br', life: 3000});
    }
  }
}
