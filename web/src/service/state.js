import { reactive } from "vue";

export const connection = reactive({
  server:
    {
      value: 'Uninitialized',
      severity: 'danger',
    },
  device:
    {
      value: 'Offline',
      severity: 'danger',
    },
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
  boot: { label: "Boot", value: "N/A" },
  current: { label: "Current", value: "N/A" },
  uptime: { label: "Uptime", value: "N/A" },
});

export const cons = reactive({
  usrevt: "",
  can: {
    id: "",
    data: Array.from({ length: 8 }, () => ""),
  },
});

export const inputs = reactive({
  digital: {
    din1: { name: 'DIN1', value: false },
    din2: { name: 'DIN2', value: false },
    din3: { name: 'DIN3', value: false },
    din4: { name: 'DIN4', value: false },
  },
  analog: {
    ain1: { name: 'AIN1', value: 0, divider: false, multiplier: 1 },
    ain2: { name: 'AIN2', value: 0, divider: false, multiplier: 1 },
    ain3: { name: 'AIN3', value: 0, divider: false, multiplier: 1 },
    ain4: { name: 'AIN4', value: 0, divider: false, multiplier: 1 },
    ain5: { name: 'AIN5', value: 0, divider: false, multiplier: 1 },
    ain6: { name: 'AIN6', value: 0, divider: false, multiplier: 1 },
  },
});
