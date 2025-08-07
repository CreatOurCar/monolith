<script setup>
  import {ref} from 'vue';
  import {views, units} from '@/service/state';

  import ToastEventBus from 'primevue/toasteventbus';

  const new_unit = ref({name: '', unit: ''});

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

    units.push({name: name, unit: unit});
    new_unit.value.name = '';
    new_unit.value.unit = '';
  }

  function remove_unit(index) {
    units.splice(index, 1);
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
            <Tag v-for="(u, idx) in units" :key="idx" :value="`${u.name} (${u.unit})`" class="text-lg cursor-pointer"
              @click="remove_unit(idx)" />
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Display</div>
          <div class="text-lg font-semibold mt-2">Telemetry</div>
          <div class="grid grid-cols-12 gap-4 items-center pl-4">
            <template v-for="(view, key, idx) in views" :key="key">
              <label :for="key" class="flex items-center col-span-3" :class="{ 'col-start-7': idx % 2 === 1 }">
                {{ view.name }}
              </label>
              <div class="flex items-center col-span-3" :class="{ 'col-start-10': idx % 2 === 1 }">
                <ToggleSwitch :id="key" v-model="view.display.telemetry" />
              </div>
            </template>
          </div>

          <div class="text-lg font-semibold mt-2">Viewer</div>
          <div class="grid grid-cols-12 gap-4 items-center pl-4">
            <label class="flex items-center col-span-3">GPS</label>
            <div class="flex items-center col-span-3 col-start-4">
              <ToggleSwitch v-model="views.gps.display.viewer" />
            </div>
          </div>
        </div>
      </div>

      <div class="md:w-1/2">
        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Analog</div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">CAN</div>
        </div>
      </div>
    </div>
  </Fluid>
</template>
