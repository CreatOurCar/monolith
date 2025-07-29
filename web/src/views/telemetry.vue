<script setup>
  defineOptions({name: 'Telemetry'});

  import {ref, onMounted} from 'vue';
  import "@xterm/addon-fit";
  import "@xterm/xterm/css/xterm.css";
  import {term} from '@/service/terminal';

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
        <div class="font-semibold text-xl mb-6">Console</div>
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
</style>
