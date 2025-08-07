<script setup>
  import {ref} from 'vue';
  import {views, units} from '@/service/ui';

  import ToastEventBus from 'primevue/toasteventbus';

  const new_unit = ref({name: '', unit: ''});

  const analogs = Object.keys(views.analog.ch).filter(key => key.includes('ain'))
    .reduce((o, k) => {o[k] = views.analog.ch[k]; return o;}, {});

  function add_unit() {
    const name = new_unit.value.name.trim();
    const unit = new_unit.value.unit.trim();

    if (!name || !unit) {
      return;
    }

    if (units.some(u => u.name === name)) {
      ToastEventBus.emit('add', {severity: 'error', summary: 'Unit Already Exists', group: 'br', life: 5000});
      return;
    }

    units.push({name: name, unit: unit, display: `${name} (${unit})`});
    new_unit.value.name = '';
    new_unit.value.unit = '';
  }

  function remove_unit(index) {
    if (units[index].name !== 'Volt') {
      units.splice(index, 1);
    }

  }

</script>

<template>
  <Fluid>
    <div class="flex flex-col md:flex-row gap-8">
      <div class="md:w-1/2">
        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Units</div>
          <InputGroup class="mt-2">
            <InputText id="new_name" v-model="new_unit.name" placeholder="Name" />
            <InputText id="new_unit" v-model="new_unit.unit" placeholder="Unit" />
            <Button icon="pi pi-plus" @click="add_unit" />
          </InputGroup>
          <div class="mt-2 flex flex-wrap gap-4">
            <Tag v-for="(u, idx) in units" :key="idx" :value="u.display"
              class="text-lg cursor-pointer [user-select:none]" @click="remove_unit(idx)" />
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl mb-2">Display</div>
          <div class="text-lg font-semibold">Telemetry</div>
          <div class="grid grid-cols-12 gap-6 items-center pl-4">
            <template v-for="(view, key, idx) in views" :key="key">
              <label :for="key" class="flex items-center col-span-3 font-medium"
                :class="{ 'col-start-7': idx % 2 === 1 }">
                {{ view.name }}
              </label>
              <div class="flex items-center col-span-3" :class="{ 'col-start-10': idx % 2 === 1 }">
                <ToggleSwitch :id="key" v-model="view.display.telemetry" />
              </div>
            </template>
          </div>

          <div class="text-lg font-semibold mt-2">Viewer</div>
          <div class="grid grid-cols-12 gap-4 items-center pl-4">
            <label class="flex items-center col-span-3 font-medium">GPS</label>
            <div class="flex items-center col-span-3 col-start-4">
              <ToggleSwitch v-model="views.gps.display.viewer" />
            </div>
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl mb-2">Digital</div>
          <div class="grid grid-cols-2 gap-4">
            <div v-for="(channel, key) in views.digital.ch" :key="key" class="flex items-center gap-2">
              <label :for="key" class="w-16 font-medium">{{ key.toUpperCase() }}:</label>
              <InputText :id="key" v-model="channel.name" placeholder="Name" class="w-20" :fluid="false" />
            </div>
          </div>
        </div>
      </div>

      <div class="md:w-1/2">
        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl mb-2">Analog</div>
          <div v-for="(channel, key) in analogs" :key="key" class="flex flex-col gap-2 mb-2">
            <div class="grid grid-cols-12 gap-2 items-center">
              <label :for="`${key}-name`" class="col-span-3 font-medium">{{ key.toUpperCase() }}:</label>
              <div class="col-span-4">
                <InputText :id="`${key}-name`" v-model="channel.name" placeholder="Name" class="w-20" :fluid="false" />
              </div>
              <div v-if="['ain1','ain2','ain3','ain4'].includes(key)" class="col-span-5 flex items-center">
                <label class="mr-4">DIV</label>
                <ToggleSwitch v-model="channel.divider" />
              </div>
            </div>
            <div class="grid grid-cols-12 gap-2 items-center">
              <div class="col-span-4 flex items-center gap-2 col-start-4">
                <div class="flex items-end">
                  <input v-model.number="channel.multiplier" type="number" step="0.001"
                    class="w-20 p-inputtext p-component p-filled" />
                  <span class="mb-1">&ensp;x</span>
                </div>
              </div>
              <div class="col-span-5 flex items-center">
                <div class="flex items-center w-full">
                  <Select v-model="channel.unit" :options="units" optionLabel="display" optionValue="name"
                    placeholder="Unit" class="w-28" />
                </div>
              </div>
            </div>
            <hr v-if="key !== 'ain6'" class='mb-0' />
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">CAN</div>
        </div>
      </div>
    </div>
  </Fluid>
</template>

<style>
  input[type="number"] {
    -moz-appearance: textfield;
    -webkit-appearance: none;
    appearance: none;
  }

  input[type="number"]::-webkit-outer-spin-button,
  input[type="number"]::-webkit-inner-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
</style>
