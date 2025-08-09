<script setup>
  import {ref, reactive, computed} from 'vue';
  import {views, units} from '@/service/ui';

  import ToastEventBus from 'primevue/toasteventbus';

  const new_unit = ref({name: '', unit: ''});

  const analogs = Object.keys(views.analog.ch).filter(key => key.includes('ain'))
    .reduce((o, k) => {o[k] = views.analog.ch[k]; return o;}, {});

  const unit_options = computed(() => {
    return Object.entries(units).map(([name, {unit, display}]) => ({name, unit, display}));
  });


  const can_views = computed(() => {
    return Object.entries(views.can.view).map(([name, v]) => ({
      name,
      id: v.id,
      unit: v.unit,
      mode: v.mode,
      start: v.start,
      end: v.end,
      endian: v.mode === 'byte' ? v.endian : null,
    }))
  })

  const display_can_view = ref(false);
  const can_cfg = reactive({
    name: '',
    id: '',
    unit: null,
    mode: 'byte',
    start: 0,
    end: 0,
    endian: 'little',
  });

  const endian_options = [{label: 'Little Endian', value: 'little'}, {label: 'Big Endian', value: 'big'}];
  const bitwise_options = [{label: 'Byte', value: 'byte'}, {label: 'Bit', value: 'bit'}];
  const errors = reactive({name: '', id: '', unit: '', range: ''})

  function save_cfg() {
    can_cfg.name = can_cfg.name.trim();
    errors.name = can_cfg.name ? '' : 'Please fill in the name.'

    const id = parseInt(can_cfg.id.trim(), 16);

    errors.id = id ? '' : 'Please fill in the ID in hex format.';
    errors.id = !(id < 0 || id > (1 << 29) - 1) ? errors.id : 'ID must be within 29 bits.';

    errors.unit = can_cfg.unit ? '' : 'Please select a unit.';

    const max = can_cfg.mode === 'byte' ? 7 : 63;

    errors.range = can_cfg.start === '' || can_cfg.end === '' || can_cfg.start < 0 || can_cfg.end < 0 ||
      can_cfg.start > max || can_cfg.end > max || can_cfg.start > can_cfg.end ? `Range must be within 0 and ${max}.` : '';

    if (!errors.name && !errors.id && !errors.unit && !errors.range) {
      views.can.view[can_cfg.name] = {
        id: id,
        unit: can_cfg.unit,
        mode: can_cfg.mode,
        start: can_cfg.start,
        end: can_cfg.end,
        endian: can_cfg.mode === 'byte' ? can_cfg.endian : null,
      };

      can_cfg.name = '';
      can_cfg.id = '';
      can_cfg.unit = null;
      can_cfg.mode = 'byte';
      can_cfg.start = 0;
      can_cfg.end = 0;
      can_cfg.endian = 'little';
      display_can_view.value = false;
    }
  }

  function can_edit(name) {
    const view = views.can.view[name];
    can_cfg.name = name;
    can_cfg.id = '0x' + view.id.toString(16).toUpperCase();
    can_cfg.unit = view.unit;
    can_cfg.mode = view.mode;
    can_cfg.start = view.start;
    can_cfg.end = view.end;
    can_cfg.endian = view.endian || 'little';
    display_can_view.value = true;
  }

  function can_delete(name) {
    delete views.can.view[name];
  }

  function add_unit() {
    const name = new_unit.value.name.trim();
    const unit = new_unit.value.unit.trim();

    if (!name || !unit) {
      return;
    }

    if (units[name]) {
      ToastEventBus.emit('add', {severity: 'error', summary: 'Unit Already Exists', group: 'br', life: 5000});
      return;
    }

    units[name] = {unit: unit, display: `${name} (${unit})`};
    new_unit.value.name = '';
    new_unit.value.unit = '';
  }

  function remove_unit(key) {
    if (!units[key].default) {
      delete units[key];
    }
  }
</script>

<template>
  <Fluid>
    <div class="flex flex-col md:flex-row gap-8">
      <div class="md:w-1/2">
        <div class="card flex flex-col gap-4" style="line-height: 1.5;">
          <span><span class="pi pi-info-circle mr-2"></span> Refresh page to apply changes for the charts.</span>
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
          <div class="font-semibold text-xl">Units</div>
          <InputGroup class="mt-2">
            <InputText id="new_name" v-model="new_unit.name" placeholder="Name" />
            <InputText id="new_unit" v-model="new_unit.unit" placeholder="Unit" />
            <Button icon="pi pi-plus" @click="add_unit" />
          </InputGroup>
          <div class="mt-2 flex flex-wrap gap-4">
            <Tag v-for="(u, key) in units" :key="key" :value="u.display" :severity="u.default ? 'success' : 'primary'"
              class="text-lg cursor-pointer [user-select:none]" @click="remove_unit(key)" />
          </div>
        </div>

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
                  <Select v-model="channel.unit" :options="unit_options" optionLabel="display" optionValue="name"
                    placeholder="Unit" class="w-28" />
                </div>
              </div>
            </div>
            <hr v-if="key !== 'ain6'" class='mb-0' />
          </div>
        </div>

        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl mb-2">CAN</div>
          <Button label="Add Message View" icon="pi pi-plus" @click="display_can_view = true;" />
          <DataView v-if="can_views.length" :value="can_views" class="rounded-2xl">
            <template #list="{ items }">
              <div class="flex flex-col gap-2">
                <div v-for="item in items" :key="item.name" class="flex items-center justify-between border-b p-3">
                  <div class="flex flex-col">
                    <div class="text-lg font-semibold leading-tight">{{ item.name }}</div>
                    <div class="text-xs opacity-70 leading-tight">0x{{ item.id.toString(16).toUpperCase() }}</div>
                  </div>
                  <div class="flex items-center gap-2">
                    <Button icon="pi pi-pencil" size="small" severity="warn" text @click="can_edit(item.name)" />
                    <Button icon="pi pi-trash" size="small" severity="danger" text @click="can_delete(item.name)" />
                  </div>
                </div>
              </div>
            </template>
          </DataView>
        </div>
      </div>

      <Dialog v-model:visible="display_can_view" modal header="CAN Message Decoder" :style="{ width: '25rem' }">
        <div class="flex flex-col gap-4">
          <div class="grid grid-cols-12 gap-4 items-center">
            <label class="col-span-3 font-medium">Name:</label>
            <InputText v-model="can_cfg.name" class="col-span-9" placeholder="View Name" :invalid="!!errors.name" />
            <small v-if="errors.name" class="text-red-500 -mt-3 col-start-4 col-span-9">{{ errors.name }}</small>
          </div>
          <div class="grid grid-cols-12 gap-4 items-center">
            <label class="col-span-3 font-medium">ID:</label>
            <InputText v-model="can_cfg.id" class="col-span-9" placeholder="CAN msg ID (HEX)" :invalid="!!errors.id" />
            <small v-if="errors.id" class="text-red-500 -mt-3 col-start-4 col-span-9">{{ errors.id }}</small>
          </div>
          <div class="grid grid-cols-12 gap-4 items-center">
            <label class="col-span-3 font-medium">Unit:</label>
            <Select v-model="can_cfg.unit" :options="unit_options" optionLabel="display" optionValue="name"
              :invalid="!!errors.unit" placeholder="Unit" class="col-span-9" />
            <small v-if="errors.unit" class="text-red-500 -mt-3 col-start-4 col-span-9">{{ errors.unit }}</small>
          </div>
          <div class="grid grid-cols-12 gap-4 items-center">
            <label class="col-span-3 font-medium">Mode:</label>
            <SelectButton v-model="can_cfg.mode" :options="bitwise_options" optionLabel="label" optionValue="value"
              :allowEmpty="false" class="col-span-9" />
          </div>
          <div class="grid grid-cols-12 gap-4 items-center">
            <label class="col-span-3 font-medium">Range:</label>
            <div class="col-span-9 flex items-center gap-2">
              #
              <input v-model.number="can_cfg.start" type="number" class="w-12 p-inputtext p-component p-filled"
                :invalid="!!errors.range" />
              <span class="ml-2 mr-2">~</span>
              #
              <input v-model.number="can_cfg.end" type="number" class="w-12 p-inputtext p-component p-filled"
                :invalid="!!errors.range" />
            </div>
            <small v-if="errors.range" class="text-red-500 -mt-3 col-start-4 col-span-9">{{ errors.range }}</small>
          </div>
          <template v-if="can_cfg.mode === 'byte'">
            <div class="grid grid-cols-12 gap-4 items-center">
              <label class="col-span-3 font-medium">Endian:</label>
              <SelectButton v-model="can_cfg.endian" :allowEmpty="false" :options="endian_options" optionLabel="label"
                optionValue="value" class="col-span-9" />
            </div>
          </template>

          <div class="flex justify-end gap-2 mt-2">
            <Button label="Cancel" severity="secondary" @click="display_can_view = false" />
            <Button label="Save" icon="pi pi-check" @click="save_cfg" />
          </div>
        </div>
      </Dialog>

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
