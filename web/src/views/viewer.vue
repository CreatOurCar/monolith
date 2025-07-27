<script setup>
  import {ref, reactive, onMounted} from 'vue';

  import uPlot from 'uplot';
  import 'uplot/dist/uPlot.min.css';

  function parse(file) {
    console.log('Parsing file:', file);
  }

  const display = reactive({
    digital: true,
    analog: true,
    gyroscope: true,
    gps: true,
    syslog: true,
    system: true,
  });

  const data = [
    [0, 1, 2, 3, 4],
    [10, 20, 15, 30, 25]
  ];

  const chartEl = ref(null)
  let chart = null
  let resizeObserver = null;

  onMounted(() => {
    const container = chartEl.value.parentElement;
    const opts = {
      width: container.clientWidth,
      height: 300,
      scales: {
        x: {time: false},
        y: {auto: true},
      },
      axes: [
        {stroke: "#888"},
        {stroke: "#888"},
      ],
      series: [
        {},
        {label: "Value", stroke: "#1f77b4"},
      ],
    };

    chart = new uPlot(opts, data, chartEl.value);

    resizeObserver = new ResizeObserver(entries => {
      for (let entry of entries) {
        chart.setSize({width: container.clientWidth, height: 300});
      }
    });
    resizeObserver.observe(container);
  });

</script>

<template>
  <div class="grid grid-cols-12 gap-8">
    <div class="col-span-full lg:col-span-12">
      <div class="card">
        <div class="font-semibold text-xl">File</div>
        <div class="flex justify-start mt-4 w-full">
          <FileUpload ref="fileupload" mode="basic" accept=".log" @uploader="parse" customUpload chooseIcon="pi pi-file"
            chooseLabel="Select" />
        </div>
      </div>

      <div class="card">
        <div class="flex items-center justify-between mb-0">
          <div class="font-semibold text-xl">Digital</div>
          <ToggleButton v-model="display.digital" onLabel="Hide" offLabel="Show" />
        </div>
        <div v-show="display.digital" class="flex mt-4">
          <div ref="chartEl"></div>
        </div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl">Download</div>
      </div>
    </div>
  </div>
</template>
