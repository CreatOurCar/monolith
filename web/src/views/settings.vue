<script setup>
  defineOptions({name: 'Settings'});

  import {ref, onMounted} from 'vue';
  import {connection_device} from '@/service/topbar';
  import {init_mqtt, publish} from '@/service/mqtt';
  import {config} from '@/service/state';

  import {useConfirm} from "primevue/useconfirm";
  import ToastEventBus from 'primevue/toasteventbus';

  const confirm = useConfirm();

  onMounted(() => {
    config.server.addr.value = localStorage.getItem('server/addr');
    config.server.name.value = localStorage.getItem('server/name');
    config.server.key.value = localStorage.getItem('server/key');

    load_confirm();
  });

  function save(event) {
    const target = event.currentTarget.id;
    const [section, field] = target.split("/");

    config[section][field].loading = true;
    config.disabled.value = true;

    localStorage.setItem(target, config[section][field].value.trim());
    init_mqtt();

    config[section][field].loading = false;
    config.disabled.value = false;

    ToastEventBus.emit('add', {severity: 'success', summary: 'Configuration Saved', group: 'br', life: 3000});
  }

  function set_cfg(event) {
    if (connection_device.value.value !== 'Online') {
      ToastEventBus.emit('add', {
        severity: 'error',
        summary: 'Configuration Failure',
        detail: 'Device is offline.',
        group: 'br',
        life: 5000
      });
      return;
    }

    const target = event.currentTarget.id;
    const [section, field] = target.split("/");

    let payload = null;

    switch (target) {
      case 'net/ssid':
      case 'net/passwd':
      case 'dev/tz':
        payload = config[section][field].value.trim();
        break;

      case 'can/filter':
      case 'can/mask':
        const buf = new ArrayBuffer(4);
        const view = new DataView(buf);
        view.setUint32(0, parseInt(config[section][field].value.trim(), 16), true);
        payload = new Uint8Array(buf);
        break;

      case 'anl/en':
      case 'dgt/en':
      case 'can/en':
      case 'gps/en':
        config[section][field].value = !config[section][field].value; // toggle value
        payload = new Uint8Array([config[section][field].value ? 1 : 0]);
        break;

      default:
        payload = new Uint8Array([config[section][field].value]);
        break;
    }

    config[section][field].loading = true;
    config.current_loading.value = target;
    config.disabled.value = true;

    publish(`set/${target}`, payload, 1);
  }

  function load_confirm() {
    confirm.require({
      header: 'Load Confirmation',
      message: 'Loading configuration may affect the logging performance. Make sure the device is not in the critical session.',
      icon: 'pi pi-info-circle',
      rejectProps: {
        label: 'Cancel',
        severity: 'secondary',
        outlined: true,
      },
      acceptProps: {
        label: 'Load',
        severity: 'success',
      },
      accept: () => {
        publish('cmd/cfg', '!', 1)
      }
    });
  }

  function restart_confirm() {
    confirm.require({
      header: 'Restart Confirmation',
      message: 'The device will be rebooted.',
      icon: 'pi pi-exclamation-triangle',
      rejectProps: {
        label: 'Cancel',
        severity: 'secondary',
        outlined: true,
      },
      acceptProps: {
        label: 'Restart',
        severity: 'warn',
      },
      accept: () => {
        publish('cmd/rbt', '!', 1);
      },
    });
  }

  function reset_confirm() {
    confirm.require({
      header: 'Reset Confirmation',
      message: 'This button has the same effect as pressing the RST button on the device. All configurations will be erased and the device need to be configured from the beginning.',
      icon: 'pi pi-exclamation-triangle',
      rejectProps: {
        label: 'Cancel',
        severity: 'secondary',
        outlined: true,
      },
      acceptProps: {
        label: 'Reset',
        severity: 'danger',
      },
      accept: () => {
        publish('cmd/rst', '!', 1);
      },
    });
  }

  const canbps = [
    {name: '1 kbit/s', value: 0},
    {name: '5 kbit/s', value: 1},
    {name: '10 kbit/s', value: 2},
    {name: '12.5 kbit/s', value: 3},
    {name: '16 kbit/s', value: 4},
    {name: '20 kbit/s', value: 5},
    {name: '25 kbit/s', value: 6},
    {name: '50 kbit/s', value: 7},
    {name: '100 kbit/s', value: 8},
    {name: '125 kbit/s', value: 9},
    {name: '250 kbit/s', value: 10},
    {name: '500 kbit/s', value: 11},
    {name: '800 kbit/s', value: 12},
    {name: '1 Mbit/s', value: 13}
  ];

  const gpsdev = [
    {name: 'UBLOX', value: 0},
  ];

</script>

<template>
  <Fluid>
    <div class="flex flex-col md:flex-row gap-8">
      <div class="md:w-1/2">
        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Server</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="server/addr" class="flex items-center col-span-3">Address</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="config.server.addr.value" placeholder="MQTT Server Address" />
                <Button id="server/addr" type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="config.disabled"
                  :loading="config.server.addr.loading" @click="save($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="server/name" class="flex items-center col-span-3">Name</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="config.server.name.value" placeholder="Device Name" />
                <Button id="server/name" type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="config.disabled"
                  :loading="config.server.name.loading" @click="save($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="server/key" class="flex items-center col-span-3">Key</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="config.server.key.value" placeholder="Device Key" />
                <Button id="server/key" type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="config.disabled"
                  :loading="config.server.key.loading" @click="save($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Device</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="net/ssid" class="flex items-center col-span-3">SSID</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="config.net.ssid.value" placeholder="Wi-Fi SSID" />
                <Button id="net/ssid" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="config.disabled"
                  :loading="config.net.ssid.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="net/passwd" class="flex items-center col-span-3">Password</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="config.net.passwd.value" placeholder="Wi-Fi Password" />
                <Button id="net/passwd" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="config.disabled"
                  :loading="config.net.passwd.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="dev/tz" class="flex items-center col-span-3">Timezone</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="config.dev.tz.value" placeholder="POSIX timezone string" />
                <Button id="dev/tz" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="config.disabled"
                  :loading="config.dev.tz.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Inputs</div>
          <div class="grid grid-cols-12 gap-2 items-center">
            <label for="anl/en" class="flex items-center col-span-3">Analog</label>
            <div class="flex items-center col-span-3">
              <ToggleSwitch id="anl/en" v-model="config.anl.en.value" :disabled="config.disabled"
                @click="set_cfg($event)" />
            </div>
            <label for="dgt/en" class="flex items-center col-span-3 col-start-7">Digital</label>
            <div class="flex items-center col-span-3 col-start-10">
              <ToggleSwitch id="dgt/en" v-model="config.dgt.en.value" :disabled="config.disabled"
                @click="set_cfg($event)" />
            </div>
          </div>
        </div>
      </div>

      <div class="md:w-1/2">
        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">CAN</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/en" class="flex items-center col-span-3">Enabled</label>
            <div class="col-span-9">
              <ToggleSwitch id="can/en" v-model="config.can.en.value" :disabled="config.disabled"
                @click="set_cfg($event)" />
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/bps" class="flex items-center col-span-3">Bit rate</label>
            <div class="col-span-9">
              <InputGroup>
                <Select v-model="config.can.bps.value" :options="canbps" optionLabel="name" optionValue="value"
                  placeholder="CAN bps" />
                <Button id="can/bps" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="config.disabled"
                  :loading="config.can.bps.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/filter" class="flex items-center col-span-3">Filter</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="config.can.filter.value" placeholder="CAN filter ID" />
                <Button id="can/filter" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="config.disabled"
                  :loading="config.can.filter.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/mask" class="flex items-center col-span-3">Mask</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="config.can.mask.value" placeholder="CAN filter mask" />
                <Button id="can/mask" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="config.disabled"
                  :loading="config.can.mask.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">GPS</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="gps/en" class="flex items-center col-span-3">Enabled</label>
            <div class="col-span-9">
              <ToggleSwitch id="gps/en" v-model="config.gps.en.value" :disabled="config.disabled"
                @click="set_cfg($event)" />
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="gps/dev" class="flex items-center col-span-3">Unit</label>
            <div class="col-span-9">
              <InputGroup>
                <Select v-model="config.gps.dev.value" :options="gpsdev" optionLabel="name" optionValue="value"
                  placeholder="GPS device" />
                <Button id="gps/dev" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="config.disabled"
                  :loading="config.gps.dev.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Danger Zone</div>
          <span>Restart the device to apply changes.</span>

          <div class="flex gap-4">
            <Button class="flex-1" label="Refresh" icon="pi pi-sync" severity="info" @click="load_confirm" />
            <Button class="flex-1" label="Restart" icon="pi pi-refresh" severity="warn" @click="restart_confirm" />
            <Button class="flex-1" label="Reset" icon="pi pi-sparkles" severity="danger" @click="reset_confirm" />
          </div>

          <ConfirmDialog style="maxWidth: 400px" />
        </div>
      </div>
    </div>
  </Fluid>
</template>

<style>
  .p-confirmdialog-message {
    line-height: 1.5;
  }
</style>
