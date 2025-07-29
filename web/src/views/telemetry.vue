<script setup>
  defineOptions({name: 'Telemetry'});

  import {ref, onMounted} from 'vue';
  import {state} from '@/service/state';
  import {term} from '@/service/terminal';

  import "@xterm/addon-fit";
  import "@xterm/xterm/css/xterm.css";

  const terminal = ref(null);

  onMounted(() => {
    const fit = new FitAddon.FitAddon();
    term.loadAddon(fit);
    term.open(terminal.value);
    fit.fit();
    window.addEventListener('resize', () => fit.fit());
  });
</script>

<template>
  <div class="grid grid-cols-12 gap-8">
    <div class="col-span-full lg:col-span-12">
      <div class="card">
        <div class="font-semibold text-xl mb-6">System State</div>

        <div class="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4 mb-4">
          <!-- Boot Time: 행 전체 차지 -->
          <div class="col-span-full flex items-center font-medium">
            <span class="w-24">Boot</span>
            <span class="ml-2">{{ boot }}</span>
          </div>
          <!-- Current : 행 전체 차지 -->
          <div class="col-span-full flex items-center font-medium">
            <span class="w-24">Current</span>
            <span class="ml-2">{{ current }}</span>
          </div>
        </div>

        <div class="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4 text-sm">
          <template v-for="item in state" :key="item.name">
            <div v-if="item.name" class="flex items-center">
              <span class="w-16">{{ item.name }}</span>
              <Tag :value="item.text" :severity="item.status" class="ml-2" />
            </div>
          </template>
        </div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">System Logs</div>
        <div ref="terminal" class="text-sm"></div>
      </div>
    </div>
  </div>
</template>

<style>
  .xterm {
    padding: 0.5rem 0.7rem;
    border-radius: 1rem;
    height: 15rem;
  }

  .xterm-viewport {
    border-radius: 0.5rem;
    height: 100%;
  }

  .p-tag-label {
    font-size: 0.75rem;
  }
</style>
