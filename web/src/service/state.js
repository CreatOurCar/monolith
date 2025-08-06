import { reactive } from "vue";
import dayjs from 'dayjs/esm';

export function format_size(size) {
  if (size >= 1024 * 1024) return (size / (1024 * 1024)).toFixed(2) + " MB"
  if (size >= 1024) return (size / 1024).toFixed(2) + " KB"
  return size + " B"
}

export const connection = reactive({
  server: { value: 'Uninitialized', severity: 'danger' },
  device: { value: 'Offline', severity: 'danger' },
  status: 'text-gray-500',
});

export const config = reactive({
  server: {
    addr: { value: "", loading: false },
    name: { value: "", loading: false },
    key: { value: "", loading: false },
  },
  net: {
    ssid: { value: "", loading: false },
    passwd: { value: "", loading: false },
  },
  dev: {
    tz: { value: "", loading: false },
  },
  can: {
    en: { value: false, loading: false },
    bps: { value: 0, loading: false },
    filter: { value: "", loading: false },
    mask: { value: "", loading: false },
  },
  gps: {
    en: { value: false, loading: false },
    dev: { value: 0, loading: false },
  },
  anl: {
    en: { value: false, loading: false },
  },
  dgt: {
    en: { value: false, loading: false },
  },
  current_loading: "",
  disabled: false,
});

export const files = reactive({
  buf: [],
  list: [],
  loading: {
    list: false,
    del: false,
    download: false,
  },
  disabled: false,
  download: {
    name: "",
    buf: [],
    progress: 0,
    size: 0,
  }
});

export const state = reactive([
  { name: '', text: 'UNKNOWN', status: "secondary" }, // hide core state
  { name: 'NVS', text: 'UNKNOWN', status: "secondary" },
  { name: 'RTC', text: 'UNKNOWN', status: "secondary" },
  { name: 'SD', text: 'UNKNOWN', status: "secondary" },
  { name: 'WIFI', text: 'UNKNOWN', status: "secondary" },
  { name: 'MQTT', text: 'UNKNOWN', status: "secondary" },
  { name: 'CAN', text: 'UNKNOWN', status: "secondary" },
  { name: 'GPS', text: 'UNKNOWN', status: "secondary" },
  { name: 'ANALOG', text: 'UNKNOWN', status: "secondary" },
  { name: 'DIGITAL', text: 'UNKNOWN', status: "secondary" },
  { name: 'GYRO', text: 'UNKNOWN', status: "secondary" },
]);

export const times = reactive({
  boot: { label: "Boot", value: "-", raw: null },
  current: { label: "Current", value: "-" },
  uptime: { label: "Uptime", value: "-" },
});

export const cons = reactive({
  usrevt: "",
  can: { id: "", data: Array.from({ length: 8 }, () => "") },
});

export const views = reactive({
  digital: {
    display: true,
    ch: {
      din1: { name: 'DIN1', value: false },
      din2: { name: 'DIN2', value: false },
      din3: { name: 'DIN3', value: false },
      din4: { name: 'DIN4', value: false },
    }
  },
  analog: {
    display: true,
    ch: {
      ain1: { name: 'AIN1', value: 0, divider: false, multiplier: 1 },
      ain2: { name: 'AIN2', value: 0, divider: false, multiplier: 1 },
      ain3: { name: 'AIN3', value: 0, divider: false, multiplier: 1 },
      ain4: { name: 'AIN4', value: 0, divider: false, multiplier: 1 },
      ain5: { name: 'AIN5', value: 0, multiplier: 1 },
      ain6: { name: 'AIN6', value: 0, multiplier: 1 },
      volt: { name: 'PWR', value: 0, multiplier: 15430 / 430 },
      temp: { name: 'TEMP', value: 0, multiplier: 0.01 },
    }
  },
  gyro: { display: true },
  gps: { display: true },
  can: {
    // TODO:
  }
});

export const telemetry = reactive({
  chart: {},
  analog: [[], [], [], [], [], [], [], [], []],
  gyro: [[], [], [], [], [], [], []],
});

export const digit = num => num.toFixed(Math.max(0, 3 - Math.trunc(Math.abs(num)).toString().length));

export const fmt = {
  time: (u, v, sidx, didx) => {
    const d = u.data[sidx];

    if (didx == null && d) {
      v = d[d.length - 1];
    }

    return dayjs(v * 1000).format('HH:mm:ss.SSS');
  },
  digital: (u, v, sidx, didx) => {
    const d = u.data[sidx];

    if (didx == null && d) {
      v = d[d.length - 1];
    }

    switch (v) {
      case 0: return 'LOW';
      case 1: return 'HIGH';
      default: return '-';
    }
  },
  volt: (u, v, sidx, didx) => {
    const d = u.data[sidx];

    if (didx == null && d) {
      v = d[d.length - 1];
    }

    if (isNaN(v) || !v) {
      return '-';
    }

    return `${digit(v)} V`;
  },
  temp: (u, v, sidx, didx) => {
    const d= u.data[sidx];

    if (didx == null && d) {
      v = d[d.length - 1];
    }

    if (isNaN(v) || !v) {
      return '-';
    }

    return `${digit(v)} °C`;
  },
  accel: (u, v, sidx, didx) => {
    const d = u.data[sidx];

    if (didx == null && d) {
      v = d[d.length - 1];
    }

    if (isNaN(v) || !v) {
      return '-';
    }

    return `${digit(v)} g`;
  },
  gyro: (u, v, sidx, didx) => {
    const d = u.data[sidx];

    if (didx == null && d) {
      v = d[d.length - 1];
    }

    if (isNaN(v) || !v) {
      return '-';
    }

    return `${digit(v)} °/s`;
  },
  speed: (u, v, sidx, didx) => {
    const d = u.data[sidx];

    if (didx == null && d) {
      v = d[d.length - 1];
    }

    if (isNaN(v) || !v) {
      return '-';
    }

    return `${digit(v)} km/h`;
  },
}
