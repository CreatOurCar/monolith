import { reactive, watch } from 'vue';

export const views = reactive({
  digital: {
    name: 'Digital',
    display: { telemetry: true },
    ch: {
      din1: { name: 'DIN1', value: false },
      din2: { name: 'DIN2', value: false },
      din3: { name: 'DIN3', value: false },
      din4: { name: 'DIN4', value: false },
    }
  },
  analog: {
    name: 'Analog',
    display: { telemetry: true },
    ch: {
      ain1: { name: 'AIN1', value: 0, divider: false, multiplier: 1, unit: 'Volt' },
      ain2: { name: 'AIN2', value: 0, divider: false, multiplier: 1, unit: 'Volt' },
      ain3: { name: 'AIN3', value: 0, divider: false, multiplier: 1, unit: 'Volt' },
      ain4: { name: 'AIN4', value: 0, divider: false, multiplier: 1, unit: 'Volt' },
      ain5: { name: 'AIN5', value: 0, multiplier: 1, unit: 'Volt' },
      ain6: { name: 'AIN6', value: 0, multiplier: 1, unit: 'Volt' },
      volt: { name: 'PWR', value: 0, multiplier: 15430 / 430 },
      temp: { name: 'TEMP', value: 0, multiplier: 0.01 },
    }
  },
  gyro: {
    name: 'Gyro',
    display: { telemetry: true },
  },
  can: {
    name: 'CAN',
    display: { telemetry: true }
  },
  gps: {
    name: 'GPS',
    display: { telemetry: true, viewer: true },
  },
});

export const units = reactive([{ name: 'Volt', unit: 'V', display: 'Volt (V)' }]);

export function save_view() {
  localStorage.setItem('views', JSON.stringify(views));
  localStorage.setItem('units', JSON.stringify(units));
}

export function load_view() {
  const v = localStorage.getItem('views');

  if (v) {
    const parsed = JSON.parse(v);

    for (const key in parsed) {
      if (views[key]) {
        Object.assign(views[key], parsed[key]);
      } else {
        views[key] = parsed[key];
      }
    }
  }

  const u = localStorage.getItem('units');

  if (u) {
    const parsed = JSON.parse(u)
    units.splice(0, units.length, ...parsed);
  }
}

watch(views, save_view, { deep: true })
watch(units, save_view, { deep: true })

load_view();
