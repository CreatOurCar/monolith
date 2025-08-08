<script setup>
  defineOptions({name: 'Viewer'});

  import {ref, onMounted} from 'vue';
  import {dark} from '@/layout/composables/layout';
  import {parse, convert} from '@/service/protocol';
  import {fmt, digit, format_size} from '@/service/state';
  import {views, units, colors} from '@/service/ui';
  import {init_map} from '@/service/map';

  import uPlot from 'uplot';
  import 'uplot/dist/uPlot.min.css';

  import dayjs from 'dayjs/esm';

  const file = {
    device: ref(""),
    name: ref(""),
    boot: ref(""),
    duration: ref(""),
    statistic: ref(""),
  };

  let bt = 0;

  const gps = ref(null);
  const map = ref(null);
  const line = ref(null);
  const path = ref([]);
  const path_history = [];
  const slider = ref(0);
  const timelapse_pos = ref(null);
  const timelapse_time = ref("00:00:00.000");
  const timelapse_coord = ref("00.0000000, 000.0000000");
  const timelapse_speed = ref("0 km/h");
  const timelapse_course = ref("0 °");

  const chart = ref(null);
  const graph = ref(null);
  const container = ref(null);

  let events = ref([]);

  const show = {
    digital: {name: "DIN", ref: ref(false)},
    analog: {name: "AIN", ref: ref(false)},
    gyro: {name: "Gyro", ref: ref(false)},
    can: {name: "CAN", ref: ref(false)},
    gps: {name: "GPS", ref: ref(false)},
  };

  const axis = {
    volt: {splits: [], min: 0, max: 0},
    temp: {splits: [], min: 0, max: 0},
    accel: {splits: [], min: 0, max: 0},
    gyro: {splits: [], min: 0, max: 0},
    speed: {splits: [], min: 0, max: 0},
  };

  onMounted(() => {
    if (!window.kakao) {
      const script = document.createElement("script");
      script.type = 'text/javascript';
      script.src = "//dapi.kakao.com/v2/maps/sdk.js?appkey=5a6908c6e8974084c9c219f330401972&autoload=false";
      script.onload = () => init_map(map, line, path, gps);
      document.head.appendChild(script);
    } else {
      init_map(map, line, path, gps);
    }
  });

  function upload(f) {
    file.name.value = f.files[0].name;

    const reader = new FileReader();
    reader.readAsArrayBuffer(f.files[0]);
    reader.onload = (e) => {
      let result;
      try {
        result = parse(new Uint8Array(e.target.result));
      } catch (e) {
        return;
      }

      bt = result.header.boot.boot_time;
      file.device.value = result.header.boot.mac;
      file.boot.value = dayjs(bt * 1000).format('YYYY-MM-DD HH:mm:ss (UTC Z)');
      file.statistic.value = `${result.ok.toLocaleString()} valid / ${result.error.length.toLocaleString()} error (${format_size(f.files[0].size)})`;

      const d = dayjs.duration(result.latest.timestamp);
      const hours = Math.floor(d.asHours());
      const minutes = d.minutes();
      const seconds = d.seconds();

      file.duration.value = '';

      if (hours > 0) file.duration.value += `${hours} hr `;
      if (minutes > 0) file.duration.value += `${minutes} min `;
      file.duration.value += `${seconds} sec`;

      events.value = [];

      if (chart.value) {
        chart.value.destroy();
      }

      const dataset = [[], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], []];

      for (const data of result.data) {
        switch (data.type) {
          case "DIGITAL":
            dataset[0].push(bt + data.timestamp / 1000);
            dataset[1].push(data.digital.din1);
            dataset[2].push(data.digital.din2);
            dataset[3].push(data.digital.din3);
            dataset[4].push(data.digital.din4);

            for (let i = 5; i < dataset.length; i++) {
              dataset[i].push(null);
            }
            break;

          case "ANALOG":
            dataset[0].push(bt + data.timestamp / 1000);
            dataset[5].push(convert.adc_to_v(data.analog.ain1) * views.analog.ch.ain1.multiplier * (views.analog.ch.ain1.devider ? 0.5 : 1));
            dataset[6].push(convert.adc_to_v(data.analog.ain2) * views.analog.ch.ain2.multiplier * (views.analog.ch.ain2.devider ? 0.5 : 1));
            dataset[7].push(convert.adc_to_v(data.analog.ain3) * views.analog.ch.ain3.multiplier * (views.analog.ch.ain3.devider ? 0.5 : 1));
            dataset[8].push(convert.adc_to_v(data.analog.ain4) * views.analog.ch.ain4.multiplier * (views.analog.ch.ain4.devider ? 0.5 : 1));
            dataset[9].push(convert.adc_to_v(data.analog.ain5) * views.analog.ch.ain5.multiplier);
            dataset[10].push(convert.adc_to_v(data.analog.ain6) * views.analog.ch.ain6.multiplier);
            dataset[11].push(convert.adc_to_v(data.analog.voltage) * views.analog.ch.volt.multiplier);
            dataset[12].push(data.analog.temperature * views.analog.ch.temp.multiplier);

            for (let i = 1; i < dataset.length; i++) {
              if (i < 5 || i > 12) {
                dataset[i].push(null);
              }
            }
            break;

          case "GYROSCOPE":
            dataset[0].push(bt + data.timestamp / 1000);
            dataset[13].push(convert.accel_to_g(data.gyro.accel_x));
            dataset[14].push(convert.accel_to_g(data.gyro.accel_y));
            dataset[15].push(convert.accel_to_g(data.gyro.accel_z));
            dataset[16].push(convert.gyro_to_dps(data.gyro.gyro_x));
            dataset[17].push(convert.gyro_to_dps(data.gyro.gyro_y));
            dataset[18].push(convert.gyro_to_dps(data.gyro.gyro_z));

            for (let i = 1; i < dataset.length; i++) {
              if (i < 13 || i > 18) {
                dataset[i].push(null);
              }
            }
            break;

          case "CAN":
            // TODO:
            break;

          case "GPS":
            dataset[0].push(bt + data.timestamp / 1000);
            dataset[19].push(data.gps.speed);

            for (let i = 1; i < 19; i++) {
              dataset[i].push(null);
            }

            const pos = new kakao.maps.LatLng(data.gps.latitude, data.gps.longitude);
            path.value.push(pos);
            path_history.push(data);
            break;

          case "SYSTEM":
          case "USER_EVENT":
            events.value.push({
              time: dayjs(bt * 1000 + data.timestamp).format('HH:mm:ss.SSS'),
              type: data.type === "SYSTEM" ? "SYS" : "USR",
              msg: data.type === "SYSTEM" ? data.sys.msg : data.user.msg,
            });
            break;
        }
      }

      line.value.setPath(path.value);
      map.value.setCenter(path.value[0]);

      new kakao.maps.Circle({
        center: path.value[0],
        fillColor: '#00FF00',
        strokeColor: '#00FF00',
        fillOpacity: 1,
        strokeOpacity: 1,
        radius: 0.5,
      }).setMap(map.value);

      new kakao.maps.Circle({
        center: path.value[path.value.length - 1],
        fillColor: '#FF0000',
        strokeColor: '#FF0000',
        fillOpacity: 1,
        strokeOpacity: 1,
        radius: 0.5,
      }).setMap(map.value);

      timelapse_pos.value = new kakao.maps.Circle({
        fillColor: '#FF00FF',
        strokeColor: '#FF00FF',
        fillOpacity: 1,
        strokeOpacity: 1,
        radius: 0.5,
        zIndex: 2,
      });

      timelapse_pos.value.setMap(map.value);
      timelapse_pos.value.setPosition(path.value[0]);

      timelapse_time.value = dayjs(bt * 1000 + path_history[0].timestamp).format('HH:mm:ss.SSS');
      timelapse_coord.value = `${path_history[0].gps.latitude.toFixed(7)}, ${path_history[0].gps.longitude.toFixed(7)}`;
      timelapse_speed.value = `${digit(path_history[0].gps.speed)} km/h`;
      timelapse_course.value = `${digit(path_history[0].gps.course)} °`;

      init_chart(dataset);
    };
  }

  function init_chart(dataset) {
    chart.value = new uPlot({
      width: 600, height: 400,
      cursor: {
        hover: {
          prox: 10,
          bias: 0,
          skip: [null],
        }
      },
      scales: {
        digital: {
          range: (u, d_min, d_max) => {
            if (d_min === null && d_max === null) {
              return [null, null];
            } else {
              return [0, 1];
            }
          }
        },
        volt: {
          range: (u, d_min, d_max) => {
            if (d_min === null && d_max === null) {
              return [null, null];
            } else {
              axis.volt = split_range(d_min, d_max);
              return [axis.volt.min, axis.volt.max];
            }
          }
        },
        temp: {
          range: (u, d_min, d_max) => {
            if (d_min === null && d_max === null) {
              return [null, null];
            } else {
              axis.temp = split_range(d_min, d_max);
              return [axis.temp.min, axis.temp.max];
            }
          }
        },
        accel: {
          range: (u, d_min, d_max) => {
            if (d_min === null && d_max === null) {
              return [null, null];
            } else {
              axis.accel = split_range(d_min, d_max);
              return [axis.accel.min, axis.accel.max];
            }
          }
        },
        gyro: {
          range: (u, d_min, d_max) => {
            if (d_min === null && d_max === null) {
              return [null, null];
            } else {
              axis.gyro = split_range(d_min, d_max);
              return [axis.gyro.min, axis.gyro.max];
            }
          }
        },
        speed: {
          range: (u, d_min, d_max) => {
            if (d_min === null && d_max === null) {
              return [null, null];
            } else {
              axis.speed = split_range(d_min, d_max);
              return [axis.speed.min, axis.speed.max];
            }
          }
        },
      },
      series: [
        {value: fmt.time},
        {label: views.digital.ch.din1.name, value: fmt.digital, scale: 'digital', spanGaps: true, show: false, stroke: colors[0]},
        {label: views.digital.ch.din2.name, value: fmt.digital, scale: 'digital', spanGaps: true, show: false, stroke: colors[1]},
        {label: views.digital.ch.din3.name, value: fmt.digital, scale: 'digital', spanGaps: true, show: false, stroke: colors[2]},
        {label: views.digital.ch.din4.name, value: fmt.digital, scale: 'digital', spanGaps: true, show: false, stroke: colors[3]},
        {label: views.analog.ch.ain1.name, value: fmt.volt, scale: 'volt', spanGaps: true, show: false, stroke: colors[4]},
        {label: views.analog.ch.ain2.name, value: fmt.volt, scale: 'volt', spanGaps: true, show: false, stroke: colors[5]},
        {label: views.analog.ch.ain3.name, value: fmt.volt, scale: 'volt', spanGaps: true, show: false, stroke: colors[6]},
        {label: views.analog.ch.ain4.name, value: fmt.volt, scale: 'volt', spanGaps: true, show: false, stroke: colors[7]},
        {label: views.analog.ch.ain5.name, value: fmt.volt, scale: 'volt', spanGaps: true, show: false, stroke: colors[8]},
        {label: views.analog.ch.ain6.name, value: fmt.volt, scale: 'volt', spanGaps: true, show: false, stroke: colors[9]},
        {label: views.analog.ch.volt.name, value: fmt.volt, scale: 'volt', spanGaps: true, show: false, stroke: colors[10]},
        {label: views.analog.ch.temp.name, value: fmt.temp, scale: 'temp', spanGaps: true, show: false, stroke: colors[11]},
        {label: "Ax", value: fmt.accel, scale: 'accel', spanGaps: true, show: false, stroke: colors[12]},
        {label: "Ay", value: fmt.accel, scale: 'accel', spanGaps: true, show: false, stroke: colors[13]},
        {label: "Az", value: fmt.accel, scale: 'accel', spanGaps: true, show: false, stroke: colors[14]},
        {label: "Gx", value: fmt.gyro, scale: 'gyro', spanGaps: true, show: false, stroke: colors[15]},
        {label: "Gy", value: fmt.gyro, scale: 'gyro', spanGaps: true, show: false, stroke: colors[16]},
        {label: "Gz", value: fmt.gyro, scale: 'gyro', spanGaps: true, show: false, stroke: colors[17]},
        {label: "SPD", value: fmt.speed, scale: 'speed', spanGaps: true, show: false, stroke: colors[18]},
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
          scale: 'accel',
          values: (u, v) => v.map(x => `${digit(x)}g`),
          splits: () => axis.accel.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'gyro',
          side: 1,
          size: 60,
          values: (u, v) => v.map(x => `${digit(x)}°/s`),
          splits: () => axis.gyro.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'volt',
          values: (u, v) => v.map(x => `${digit(x)}V`),
          splits: () => axis.volt.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {show: false},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'temp',
          side: 1,
          values: (u, v) => v.map(x => `${digit(x)}°C`),
          splits: () => axis.temp.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {show: false},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'digital',
          side: 1,
          size: 20,
          values: (u, v) => v.map(x => x ? "HI" : "LO"),
          splits: () => [0, 1],
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {show: false},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        },
        {
          scale: 'speed',
          side: 1,
          size: 60,
          values: (u, v) => v.map(x => `${digit(x)}km/h`),
          splits: () => axis.speed.splits,
          stroke: () => dark.value ? '#fff' : '#000',
          ticks: {show: false},
          grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        }
      ]
    }, dataset, graph.value);

    new ResizeObserver(entries => {
      for (let entry of entries) {
        chart.value.setSize({
          width: entry.contentRect.width,
          height: entry.contentRect.width * 0.6
        });
      }
    }).observe(container.value);
  }

  function toggle_axis(key) {
    switch (key) {
      case 'digital':
        for (let i = 1; i <= 4; i++) {
          chart.value.setSeries(i, {show: show[key].ref});
        }
        break;
      case 'analog':
        for (let i = 5; i <= 12; i++) {
          chart.value.setSeries(i, {show: show[key].ref});
        }
        break;
      case 'gyro':
        for (let i = 13; i <= 18; i++) {
          chart.value.setSeries(i, {show: show[key].ref});
        }
        break;
      case 'can':
        break;
      case 'gps':
        chart.value.setSeries(19, {show: show[key].ref});
        break;
    }
  }

  function split_range(d_min, d_max) {
    if (d_min === d_max) {
      d_min *= 0.85;
      d_max *= 1.15;
    }

    const tick = 5;
    const step = (d_max - d_min) / (tick - 1);
    const min = Math.floor(d_min / step) * step;
    const max = min + step * tick;
    const splits = Array.from({length: tick + 1}, (_, i) => min + i * step);
    return {min, max, splits};
  }

  Array.prototype.max = function () {
    return Math.max.apply(null, this.filter(x => x));
  };

  Array.prototype.min = function () {
    return Math.min.apply(null, this.filter(x => x));
  };

  function timelapse() {
    if (!path.value.length || !timelapse_pos.value) {
      return;
    }

    const pos = Math.floor((path.value.length - 1) * slider.value / 100);
    timelapse_pos.value.setPosition(path.value[pos]);
    timelapse_time.value = dayjs(bt * 1000 + path_history[pos].timestamp).format('HH:mm:ss.SSS');
    timelapse_coord.value = `${path_history[pos].gps.latitude.toFixed(7)}, ${path_history[pos].gps.longitude.toFixed(7)}`;
    timelapse_speed.value = `${digit(path_history[pos].gps.speed)} km/h`;
    timelapse_course.value = `${digit(path_history[pos].gps.course)} °`;
  }
</script>

<template>
  <div class="grid grid-cols-12 gap-8">
    <div class="col-span-full lg:col-span-12">
      <div class="card">
        <div class="font-semibold text-xl">File</div>
        <div class="flex justify-start mt-4 w-full" style="align-items: center;">
          <FileUpload mode="basic" accept=".log" @uploader="upload" :auto="true" customUpload chooseIcon="pi pi-file"
            chooseLabel="Select" />
          <span class="ml-4 "> {{ file.name.value ? file.name : "Select a file to view" }}</span>
        </div>
        <div v-if="file.name" class="mt-4 space-y-4">
          <div class="flex">
            <span class="font-semibold w-24">Device:</span>
            <span class="flex-1">{{ file.device }}</span>
          </div>
          <div class="flex">
            <span class="font-semibold w-24">Logs:</span>
            <span class="flex-1">{{ file.statistic }}</span>
          </div>
          <div class="flex">
            <span class="font-semibold w-24">Boot:</span>
            <span class="flex-1">{{ file.boot }}</span>
          </div>
          <div class="flex">
            <span class="font-semibold w-24">Duration:</span>
            <span class="flex-1">{{ file.duration }}</span>
          </div>
        </div>
      </div>

      <div class="card" ref="container">
        <div class="font-semibold text-xl mb-6">Graph</div>
        <div v-show="chart">
          <div class="flex flex-wrap justify-start items-center mb-6 gap-3">
            <ToggleButton v-for="(tag, key) in show" :key="key" v-model="tag.ref" :onLabel="tag.name"
              :offLabel="tag.name" @click="toggle_axis(key)" />
          </div>
          <div class="chart" ref="graph"></div>
        </div>
        <div v-show="!chart" class="text-center text-gray-400">
          Please select a file to view the graph.
        </div>
      </div>

      <div v-if="views.gps.display.viewer" class="card">
        <div class="font-semibold text-xl mb-6">GPS</div>
        <div class="flex items-center mb-6">
          <span class="font-semibold mr-6">{{ timelapse_time }}</span>
          <Slider v-model="slider" :step="0.01" class="w-full" @change="timelapse" />
        </div>
        <div class="mb-6 space-y-4">
          <div class="flex">
            <span class="font-semibold w-24">Coord:</span>
            <Tag :value="timelapse_coord" severity="info" />
          </div>
          <div class="flex">
            <span class="font-semibold w-24">Speed:</span>
            <Tag :value="timelapse_speed" severity="info" />
          </div>
          <div class="flex">
            <span class="font-semibold w-24">Course:</span>
            <Tag :value="timelapse_course" severity="info" />
          </div>
        </div>
        <div ref="gps" style="width: 100%; aspect-ratio: 1 / 0.7;"></div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">Events</div>
        <DataView :value="events">
          <template #empty>
            <div class="p-4 text-center text-gray-400">
              No events found.
            </div>
          </template>
          <template #list="slotProps">
            <div class="flex flex-col">
              <div v-for="(item, index) in slotProps.items" :key="index" class="flex items-center py-2 px-2 gap-2">
                <div class="w-8 text-center">{{ index + 1 }}</div>
                <Tag :value="item.time" severity="success" class="timetag" />
                <Tag :value="item.type" severity="info" class="timetag" />
                <Tag :value="item.msg" severity="secondary" class="timetag" />
              </div>
            </div>
          </template>
        </DataView>
      </div>
    </div>
  </div>
</template>

<style>
  .u-legend {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(105px, 1fr));
  }

  .u-legend table,
  .u-legend tbody {
    display: contents;
  }

  .u-legend tr.u-series {
    display: flex;
    flex-direction: row;
    justify-content: flex-start;
    align-items: center;
    white-space: nowrap;
    box-sizing: border-box;
  }

  .u-legend .u-series:first-child .u-marker {
    display: none;
  }

  .u-legend tr.u-series:first-child {
    grid-column: 1 / -1;
    justify-content: center;
  }

  .u-series td,
  .u-series th {
    padding: 4px 2px;
  }
</style>
