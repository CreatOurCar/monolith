<script setup>
  defineOptions({name: 'Telemetry'});

  import {ref, onMounted} from 'vue';
  import {publish} from '@/service/mqtt';
  import {term} from '@/service/terminal';
  import {state, times, cons, inputs} from '@/service/state';

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

  function send_usrevt() {
    cons.usrevt = cons.usrevt.replace(/[^\x20-\x7E]/g, '').trim().slice(0, 16);
    publish('cmd/evt', cons.usrevt, 1);
  }

  function send_can() {
    // TODO:
  }

  function ascii_only(event) {
    if (!/^[\x20-\x7E]*$/.test(event.target.value)) {
      event.target.value = event.target.value.replace(/[^\x20-\x7E]/g, '');
    }
  }

  function hex_only(event) {
    if (!/^[0-9A-Fa-fx]*$/.test(event.target.value)) {
      event.target.value = event.target.value.replace(/[^0-9A-Fa-fx]/g, '');
    }
  }
</script>

<template>
  <div class="grid grid-cols-12 gap-8">
    <div class="col-span-full lg:col-span-12">
      <div class="card">
        <div class="font-semibold text-xl mb-6">Digital</div>

        <div class="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4">
          <template v-for="item in inputs.digital" :key="item.name">
            <div v-if="item.name" class="flex items-center">
              <span class="w-20">{{ item.name }}</span>
              <Tag :value="item.value ? 'HIGH' : 'LOW'" :severity="item.value ? 'info' : 'secondary'" />
            </div>
          </template>
        </div>

      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">Analog</div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">Gyroscope</div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">GPS</div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">Console</div>
        <div class="mb-6">
          <label>Transmit User Event</label>
          <InputGroup class="mt-4 mb-3">
            <InputText v-model="cons.usrevt" placeholder="(default: USREVT)" maxlength="16" @keyup="ascii_only" />
            <Button icon="pi pi-send" @click="send_usrevt" />
          </InputGroup>
          <Message size="small" severity="secondary" variant="simple">Only ASCII characters up to 16 bytes.</Message>
        </div>
        <div>
          <label>Transmit CAN Message</label>
          <InputGroup class="mt-4 mb-2">
            <InputText v-model="cons.can.id" placeholder="CAN Message ID" maxlength="10" @keyup="hex_only" />
            <Button icon="pi pi-send" @click="send_can" />
          </InputGroup>
          <InputGroup class="mt-4 mb-3">
            <InputText v-model="cons.can.data[n - 1]" v-for="n in 8" :key="n" :placeholder="`D${n - 1}`" maxlength="2"
              @keyup="hex_only" class="can_data" />
          </InputGroup>
          <Message size="small" severity="secondary" variant="simple">CAN msg ID and data bytes in HEX format.</Message>
        </div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">System State</div>
        <div v-for="(tag, key) in times" :key="key" class="flex items-center mb-6">
          <span class="w-24 font-medium">{{ tag.label }}</span>
          <Tag :value="tag.value" severity="info" class="timetag" />
        </div>
        <div class="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4 text-sm">
          <template v-for="item in state" :key="item.name">
            <div v-if="item.name" class="flex items-center">
              <span class="w-16">{{ item.name }}</span>
              <Tag :value="item.text" :severity="item.status" class="ml-2 state" />
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
    height: 20rem;
  }

  .xterm-viewport {
    border-radius: 0.5rem;
    height: 100%;
  }

  .state .p-tag-label {
    font-size: 0.75rem;
  }

  .timetag .p-tag-label {
    font-size: 0.95rem;
  }

  .can_data {
    font-size: 0.9rem !important;
    height: 2.25rem;
  }
</style>
