<script setup>
  import {ref} from 'vue';
  import ToastEventBus from 'primevue/toasteventbus';
  import {init_mqtt} from '@/service/mqtt';
  import {refs, disabled} from '@/service/state';

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

  const display_restart = ref(false);
  const display_reset = ref(false);

  function save(event) {
    const target = event.currentTarget.previousElementSibling.id;
    const [section, field] = target.split("/");

    refs[section][field].loading.value = true;
    disabled.value = true;

    localStorage.setItem(target, refs[section][field].value.trim());
    init_mqtt();

    refs[section][field].loading.value = false;
    disabled.value = false;

    ToastEventBus.emit('add', {severity: 'success', summary: 'Success', detail: `Configuration saved`, group: 'br', life: 3000});
  }

  function load(event) {
    const target = event.currentTarget.previousElementSibling.id.split('/');
    refs[target[0]][target[1]].loading.value = true;
    disabled.value = true;
    setTimeout(() => {
      refs[target[0]][target[1]].loading.value = false;
      disabled.value = false;
    }, 1000);
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
            <label for="server/addr" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Address</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <InputText id="server/addr" type="text" v-model="refs.server.addr.value"
                  placeholder="MQTT Server Address" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="disabled"
                  :loading="refs.server.addr.loading.value" @click="save($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="server/name" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Name</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <InputText id="server/name" type="text" v-model="refs.server.name.value" placeholder="Device Name" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="disabled"
                  :loading="refs.server.name.loading.value" @click="save($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="server/key" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Key</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <InputText id="server/key" type="text" v-model="refs.server.key.value" placeholder="Device Key" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-save" :disabled="disabled"
                  :loading="refs.server.key.loading.value" @click="save($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Device</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="device/ssid" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">SSID</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <InputText id="device/ssid" type="text" v-model="refs.device.ssid.value" placeholder="Wi-Fi SSID" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.device.ssid.loading.value" @click="load($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="device/password"
              class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Password</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <InputText id="device/password" type="text" v-model="refs.device.password.value"
                  placeholder="Wi-Fi Password" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.device.password.loading.value" @click="load($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="device/timezone"
              class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Timezone</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <InputText id="device/timezone" type="text" v-model="refs.device.timezone.value"
                  placeholder="POSIX timezone string" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.device.timezone.loading.value" @click="load($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">GPS</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="gps_enabled" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Enabled</label>
            <div class="col-span-12 md:col-span-9">
              <ToggleSwitch id="gps_enabled" v-model="refs.gps.enabled.value" :disabled="disabled" />
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="gps/dev" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Unit</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <Select id="gps/dev" v-model="refs.gps.dev.value" :options="gpsdev" optionLabel="name"
                  optionValue="value" placeholder="GPS device" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.gps.dev.loading.value" @click="load($event)" />
              </InputGroup>
            </div>
          </div>
        </div>
      </div>

      <div class="md:w-1/2">
        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">CAN</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/enabled" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Enabled</label>
            <div class="col-span-12 md:col-span-9">
              <ToggleSwitch id="can/enabled" v-model="refs.can.enabled.value" :disabled="disabled" />
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/bps" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Bit rate</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <Select id="can/bps" v-model="refs.can.bps.value" :options="canbps" optionLabel="name"
                  optionValue="value" placeholder="CAN bps" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.can.bps.loading.value" @click="load($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/filter" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Filter</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <InputText id="can/filter" type="text" v-model="refs.can.filter.value" placeholder="CAN filter ID" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.can.filter.loading.value" @click="load($event)" />
              </InputGroup>
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="can/mask" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Mask</label>
            <div class="col-span-12 md:col-span-9">
              <InputGroup>
                <InputText id="can/mask" type="text" v-model="refs.can.mask.value" placeholder="CAN filter mask" />
                <Button type="button" class="mr-2 mb-2" icon="pi pi-upload" :disabled="disabled"
                  :loading="refs.can.mask.loading.value" @click="load($event)" />
              </InputGroup>
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Inputs</div>
          <div class="grid grid-cols-12 gap-2">
            <label for="analog/enabled" class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Analog</label>
            <div class="col-span-12 md:col-span-9">
              <ToggleSwitch id="analog/enabled" v-model="refs.analog.enabled.value" :disabled="disabled" />
            </div>
          </div>
          <div class="grid grid-cols-12 gap-2">
            <label for="digital/enabled"
              class="flex items-center col-span-12 mb-2 md:col-span-3 md:mb-0">Digital</label>
            <div class="col-span-12 md:col-span-9">
              <ToggleSwitch id="digital/enabled" v-model="refs.digital.enabled.value" :disabled="disabled" />
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
