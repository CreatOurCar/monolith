<script setup>
  defineOptions({name: 'Viewer'});

  import {ref, onMounted} from 'vue';
  import {dark} from '@/layout/composables/layout';
  import {views, fmt} from '@/service/state';

  import uPlot from 'uplot';
  import 'uplot/dist/uPlot.min.css';

  import dayjs from 'dayjs/esm';

  function parse(file) {
    console.log('Parsing file:', file);
  }

  let map = ref(null);
  let gps = ref(null);
  let chart = ref(null);
  let container = ref(null);

  let dataset = [[1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1], [1]];

  onMounted(() => {
    if (!window.kakao || !window.kakao.maps) {
      const script = document.createElement("script");
      script.type = 'text/javascript';
      script.src = "//dapi.kakao.com/v2/maps/sdk.js?appkey=5a6908c6e8974084c9c219f330401972&autoload=false";
      script.onload = () => {
        window.kakao.maps.load(() => {
          map = new kakao.maps.Map(gps.value, {
            mapTypeId: kakao.maps.MapTypeId.HYBRID,
            // center: new kakao.maps.LatLng(37.2829317, 127.0435822),
            center: new kakao.maps.LatLng(35.2921728, 126.5740566),
            level: 3
          });
          map.addControl(new kakao.maps.MapTypeControl(), kakao.maps.ControlPosition.TOPRIGHT);
        });
      };
      document.head.appendChild(script);
    }

    init_chart();
  });

  const colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#e377c2", "#17becf"];

  const axis = {
    volt: {splits: [], min: 0, max: 0},
    temp: {splits: [], min: 0, max: 0},
    accel: {splits: [], min: 0, max: 0},
    gyro: {splits: [], min: 0, max: 0},
  };

  function split_range(d_min, d_max, extra_step = false) {
    const tick = 5;
    const step = (d_max - d_min) / (tick - 1);
    const min = Math.floor(d_min / step) * step;

    const tick_margin = extra_step ? tick + 1 : tick;
    const max = min + step * (tick_margin - 1);

    const splits = Array.from({length: tick_margin}, (_, i) => min + i * step);
    return {min, max, splits};
  }

  function init_chart() {
    chart = new uPlot({
      width: 600, height: 400,
      scales: {
        digital: {
          range: (u, d_min, d_max) => {
            axis.digital = split_range(d_min, d_max, true);
            return [axis.digital.min, axis.digital.max];
          }
        },
        volt: {
          range: (u, d_min, d_max) => {
            axis.volt = split_range(d_min, d_max, true);
            return [axis.volt.min, axis.volt.max];
          }
        },
        temp: {
          range: (u, d_min, d_max) => {
            axis.temp = split_range(d_min, d_max, true);
            return [axis.temp.min, axis.temp.max];
          }
        },
        accel: {
          range: (u, d_min, d_max) => {
            axis.accel = split_range(d_min, d_max, true);
            return [axis.accel.min, axis.accel.max];
          }
        },
        gyro: {
          range: (u, d_min, d_max) => {
            axis.gyro = split_range(d_min, d_max, true);
            return [axis.gyro.min, axis.gyro.max];
          }
        },
      },
      series: [
        {value: fmt.time},
        {label: views.digital.ch.din1.name, stroke: '#ff0000', value: fmt.volt, points: {show: false}, scale: 'digital'},
        {label: views.digital.ch.din2.name, stroke: '#00ff00', value: fmt.volt, points: {show: false}, scale: 'digital'},
        {label: views.digital.ch.din3.name, stroke: '#0000ff', value: fmt.volt, points: {show: false}, scale: 'digital'},
        {label: views.digital.ch.din4.name, stroke: '#ffff00', value: fmt.volt, points: {show: false}, scale: 'digital'},
        {label: views.analog.ch.ain1.name, stroke: colors[0], value: fmt.volt, points: {show: false}, scale: 'volt'},
        {label: views.analog.ch.ain2.name, stroke: colors[1], value: fmt.volt, points: {show: false}, scale: 'volt'},
        {label: views.analog.ch.ain3.name, stroke: colors[2], value: fmt.volt, points: {show: false}, scale: 'volt'},
        {label: views.analog.ch.ain4.name, stroke: colors[3], value: fmt.volt, points: {show: false}, scale: 'volt'},
        {label: views.analog.ch.ain5.name, stroke: colors[4], value: fmt.volt, points: {show: false}, scale: 'volt'},
        {label: views.analog.ch.ain6.name, stroke: colors[5], value: fmt.volt, points: {show: false}, scale: 'volt'},
        {label: views.analog.ch.volt.name, stroke: colors[6], value: fmt.volt, points: {show: false}, scale: 'volt'},
        {label: views.analog.ch.temp.name, stroke: colors[7], value: fmt.temp, points: {show: false}, scale: 'temp', show: false},
        {label: "Ax", stroke: colors[0], value: fmt.accel, points: {show: false}, scale: 'accel'},
        {label: "Ay", stroke: colors[1], value: fmt.accel, points: {show: false}, scale: 'accel'},
        {label: "Az", stroke: colors[2], value: fmt.accel, points: {show: false}, scale: 'accel'},
        {label: "Gx", stroke: colors[3], value: fmt.gyro, points: {show: false}, scale: 'gyro'},
        {label: "Gy", stroke: colors[4], value: fmt.gyro, points: {show: false}, scale: 'gyro'},
        {label: "Gz", stroke: colors[5], value: fmt.gyro, points: {show: false}, scale: 'gyro'},
      ],
      axes: [
        {
          size: 35,
          values: (u, v) => v.map(x => dayjs(x * 1000).format('HH:mm:ss')),
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'digital',
          size: 50,
          values: (u, v) => v.map(x => isNaN(x) ? 'N/A' : `${x.toFixed(1)}V`),
          splits: () => axis.digital.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'volt',
          size: 50,
          values: (u, v) => v.map(x => isNaN(x) ? 'N/A' : `${x.toFixed(1)}V`),
          splits: () => axis.volt.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'temp',
          side: 1,
          size: 50,
          values: (u, v) => v.map(x => isNaN(x) ? 'N/A' : `${x.toFixed(1)}°C`),
          splits: () => axis.temp.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'accel',
          size: 50,
          values: (u, v) => v.map(x => isNaN(x) ? 'N/A' : `${x.toFixed(1)}g`),
          splits: () => axis.accel.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'gyro',
          side: 1,
          size: 50,
          values: (u, v) => v.map(x => isNaN(x) ? 'N/A' : `${x.toFixed(0)}°/s`),
          splits: () => axis.gyro.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
      ]
    }, dataset, chart.value);

    new ResizeObserver(entries => {
      for (let entry of entries) {
        chart.setSize({
          width: entry.contentRect.width,
          height: entry.contentRect.width * 0.6
        });
      }
    }).observe(container.value);
  }

</script>

<template>
  <div class="grid grid-cols-12 gap-8">
    <div class="col-span-full lg:col-span-12">
      <div class="card">
        <div class="font-semibold text-xl">File</div>
        <div class="flex justify-start mt-4 w-full">
          <FileUpload mode="basic" accept=".log" @uploader="parse" customUpload chooseIcon="pi pi-file"
            chooseLabel="Select" />
        </div>
      </div>

      <div class="card" ref="container">
        <div class="font-semibold text-xl mb-4">Graph</div>
        <div class="chart" ref="chart"></div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-4">GPS</div>
        <div ref="gps" style="width: 100%; height: 40vh;"></div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">Events</div>
      </div>
    </div>
  </div>
</template>

<style>
  .u-legend {
    display: flex;
    flex-wrap: wrap;
    justify-content: center;
    gap: 0 0.5rem;
  }

  .u-legend table,
  .u-legend tbody {
    display: contents;
  }

  .u-legend tr.u-series {
    display: flex;
    align-items: center;
    white-space: nowrap;
    box-sizing: border-box;
    flex: 0 1;
    margin: 0;
  }

  .u-legend tr.u-series:first-child {
    flex-basis: 100%;
    justify-content: center;
  }
</style>
