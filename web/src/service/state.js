import { ref, reactive } from "vue";

export const refs = reactive({
  server: {
    addr: { value: ref(""), loading: ref(false) },
    name: { value: ref(""), loading: ref(false) },
    key: { value: ref(""), loading: ref(false) },
  },
  device: {
    ssid: { value: ref(""), loading: ref(false) },
    password: { value: ref(""), loading: ref(false) },
    timezone: { value: ref(""), loading: ref(false) },
  },
  can: {
    enabled: { value: ref(false), loading: ref(false) },
    bps: { value: ref(0), loading: ref(false) },
    filter: { value: ref(""), loading: ref(false) },
    mask: { value: ref(""), loading: ref(false) },
  },
  gps: {
    enabled: { value: ref(false), loading: ref(false) },
    dev: { value: ref(0), loading: ref(false) },
  },
  analog: {
    enabled: { value: ref(false), loading: ref(false) },
  },
  digital: {
    enabled: { value: ref(false), loading: ref(false) },
  },
});

export const disabled = ref(false);
