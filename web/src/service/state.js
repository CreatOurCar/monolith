import { ref, reactive } from "vue";

export const config = reactive({
  server: {
    addr: { value: ref(""), loading: ref(false) },
    name: { value: ref(""), loading: ref(false) },
    key: { value: ref(""), loading: ref(false) },
  },
  net: {
    ssid: { value: ref(""), loading: ref(false) },
    passwd: { value: ref(""), loading: ref(false) },
  },
  dev: {
    tz: { value: ref(""), loading: ref(false) },
  },
  can: {
    en: { value: ref(false), loading: ref(false) },
    bps: { value: ref(0), loading: ref(false) },
    filter: { value: ref(""), loading: ref(false) },
    mask: { value: ref(""), loading: ref(false) },
  },
  gps: {
    en: { value: ref(false), loading: ref(false) },
    dev: { value: ref(0), loading: ref(false) },
  },
  anl: {
    en: { value: ref(false), loading: ref(false) },
  },
  dgt: {
    en: { value: ref(false), loading: ref(false) },
  },
  current_loading: ref(""),
  disabled: ref(false),
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
