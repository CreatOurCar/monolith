<script setup>
  import {ref} from 'vue';
  import {views} from '@/service/state';

</script>

<template>
  <Fluid>
    <div class="flex flex-col md:flex-row gap-8">
      <div class="md:w-1/2">
        <div class="card flex flex-col gap-4">
          <div class="font-semibold text-xl">Units</div>
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
