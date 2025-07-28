import { ref, reactive } from "vue";

export const refs = reactive({
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
});

export const current_loading = ref("");
export const disabled = ref(false);
