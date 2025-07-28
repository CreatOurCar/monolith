<script setup>
  import {ref} from 'vue';
  import ToastEventBus from 'primevue/toasteventbus';
  import {init_mqtt, publish} from '@/service/mqtt';
  import {refs, current_loading, disabled} from '@/service/state';

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

  function save(event) {
    const target = event.currentTarget.id;
    const [section, field] = target.split("/");

    refs[section][field].loading = true;
    disabled.value = true;

    localStorage.setItem(target, refs[section][field].value.trim());
    init_mqtt();

    refs[section][field].loading = false;
    disabled.value = false;

    ToastEventBus.emit('add', {severity: 'success', summary: 'Success', detail: `Configuration saved`, group: 'br', life: 3000});
  }

  function set_cfg(event) {
    const target = event.currentTarget.id;
    const [section, field] = target.split("/");

    let payload = null;

    switch (target) {
      case 'net/ssid':
      case 'net/passwd':
      case 'dev/tz':
        payload = refs[section][field].value.trim();
        break;

      case 'can/filter':
      case 'can/mask':
        const buf = new ArrayBuffer(4);
        const view = new DataView(buf);
        view.setUint32(0, parseInt(refs[section][field].value.trim(), 16), true);
        payload = new Uint8Array(buf);
        break;

      case 'anl/en':
      case 'dgt/en':
      case 'can/en':
      case 'gps/en':
        refs[section][field].value = !refs[section][field].value; // toggle value
        payload = new Uint8Array([refs[section][field].value ? 1 : 0]);
        break;

      default:
        payload = new Uint8Array([refs[section][field].value]);
        break;
    }

    refs[section][field].loading = true;
    current_loading.value = target;
    disabled.value = true;

    publish(`set/${target}`, payload, 1);
  }

  const display_restart = ref(false);

  function restart_device() {
    publish('cmd/rbt', '!', 1);
    display_restart.value = false;
  }

  const display_reset = ref(false);

  function reset_device() {
    publish('cmd/rst', '!', 1);
    display_reset.value = false;
  }

  refs.server.addr.value = localStorage.getItem('server/addr');
  refs.server.name.value = localStorage.getItem('server/name');
  refs.server.key.value = localStorage.getItem('server/key');
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
                <InputText type="text" v-model="refs.server.addr.value" placeholder="MQTT Server Address" />
                <Button id="server/addr" type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="disabled"
                  :loading="refs.server.addr.loading" @click="save($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="server/name" class="flex items-center col-span-3">Name</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="refs.server.name.value" placeholder="Device Name" />
                <Button id="server/name" type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="disabled"
                  :loading="refs.server.name.loading" @click="save($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="server/key" class="flex items-center col-span-3">Key</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="refs.server.key.value" placeholder="Device Key" />
                <Button id="server/key" type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="disabled"
                  :loading="refs.server.key.loading" @click="save($event)" />
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
                <InputText type="text" v-model="refs.net.ssid.value" placeholder="Wi-Fi SSID" />
                <Button id="net/ssid" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.net.ssid.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="net/passwd" class="flex items-center col-span-3">Password</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="refs.net.passwd.value" placeholder="Wi-Fi Password" />
                <Button id="net/passwd" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.net.passwd.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="dev/tz" class="flex items-center col-span-3">Timezone</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="refs.dev.tz.value" placeholder="POSIX timezone string" />
                <Button id="dev/tz" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.dev.tz.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Inputs</div>
          <div class="grid grid-cols-12 gap-2 items-center">
            <label for="anl/en" class="flex items-center col-span-3">Analog</label>
            <div class="flex items-center col-span-3">
              <ToggleSwitch id="anl/en" v-model="refs.anl.en.value" :disabled="disabled" @click="set_cfg($event)" />
            </div>
            <label for="dgt/en" class="flex items-center col-span-3 col-start-7">Digital</label>
            <div class="flex items-center col-span-3 col-start-10">
              <ToggleSwitch id="dgt/en" v-model="refs.dgt.en.value" :disabled="disabled" @click="set_cfg($event)" />
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
              <ToggleSwitch id="can/en" v-model="refs.can.en.value" :disabled="disabled" @click="set_cfg($event)" />
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/bps" class="flex items-center col-span-3">Bit rate</label>
            <div class="col-span-9">
              <InputGroup>
                <Select v-model="refs.can.bps.value" :options="canbps" optionLabel="name" optionValue="value"
                  placeholder="CAN bps" />
                <Button id="can/bps" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.can.bps.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/filter" class="flex items-center col-span-3">Filter</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="refs.can.filter.value" placeholder="CAN filter ID" />
                <Button id="can/filter" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.can.filter.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/mask" class="flex items-center col-span-3">Mask</label>
            <div class="col-span-9">
              <InputGroup>
                <InputText type="text" v-model="refs.can.mask.value" placeholder="CAN filter mask" />
                <Button id="can/mask" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.can.mask.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">GPS</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="gps/en" class="flex items-center col-span-3">Enabled</label>
            <div class="col-span-9">
              <ToggleSwitch id="gps/en" v-model="refs.gps.en.value" :disabled="disabled" @click="set_cfg($event)" />
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="gps/dev" class="flex items-center col-span-3">Unit</label>
            <div class="col-span-9">
              <InputGroup>
                <Select v-model="refs.gps.dev.value" :options="gpsdev" optionLabel="name" optionValue="value"
                  placeholder="GPS device" />
                <Button id="gps/dev" type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.gps.dev.loading" @click="set_cfg($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Danger Zone</div>
          <span>Restart the device for the changes to take effect.</span>
          <div class="flex flex-wrap gap-4">
            <Button label="Restart" icon="pi pi-refresh" class="mr-2 mb-2" severity="warn" :fluid="false"
              @click="display_restart = true" raised />
            <Dialog header="Restart Confirmation" v-model:visible="display_restart" :style="{ width: '350px' }"
              :modal="true">
              <div class="flex items-center justify-center">
                <i class="pi pi-info-circle mr-2" style="font-size: 1.5rem" />
                <span>The device will be rebooted.</span>
              </div>
              <template #footer>
                <Button label="Restart Device" icon="pi pi-check" @click="restart_device" severity="warn" outlined
                  autofocus />
              </template>
            </Dialog>

            <Button label="Reset" icon="pi pi-sparkles" class="mr-2 mb-2" severity="danger" :fluid="false"
              @click="display_reset = true" raised />
            <Dialog header="Reset Confirmation" v-model:visible="display_reset" :style="{ width: '350px' }"
              :modal="true">
              <div class="flex items-center justify-center">
                <i class="pi pi-exclamation-triangle mr-6" style="font-size: 2.5rem" />
                <span style="line-height: 1.5rem;">
                  This button has the same effect as pressing the RST button on the device.<br>
                  All configurations will be erased and you should set the device from the beginning.
                </span>
              </div>
              <template #footer>
                <Button label="RESET DEVICE" icon="pi pi-check" @click="reset_device" severity="danger" outlined
                  autofocus />
              </template>
            </Dialog>
          </div>
        </div>
      </div>
    </div>
  </Fluid>
</template>
