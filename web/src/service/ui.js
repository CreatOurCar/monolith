import { reactive, watch } from 'vue';
import { parse_hex_bytes } from './protocol.js';

// === 확인 필요: 전장팀 확인 후 수정 ===
// 1. EZkontrol 프로토콜 모드: METER(1) 가정. VCU(2) 모드면 EZ_MODE = 0xD0 으로 변경.
// 2. EZkontrol SA: 기본 0xEF. 컨트롤러 번호 변경 시 EZ_SA 수정.
// 3. Daly BMS 주소: BMS_ADDR=0x01, PC=0x40 가정. 다르면 DALY_BMS_ADDR / DALY_PC 수정.
const EZ_SA         = 0xEF;
const EZ_MODE       = 0x17; // METER=0x17, VCU=0xD0
const EZ_ID1        = ((0x18 << 24) | (0x01 << 16) | (EZ_MODE << 8) | EZ_SA) >>> 0; // 0x180117EF
const EZ_ID2        = ((0x18 << 24) | (0x02 << 16) | (EZ_MODE << 8) | EZ_SA) >>> 0; // 0x180217EF
const DALY_PC       = 0x40;
const DALY_BMS_ADDR = 0x01;
const DALY_ID90     = ((0x18 << 24) | (0x90 << 16) | (DALY_PC << 8) | DALY_BMS_ADDR) >>> 0; // 0x18904001
const DALY_ID93     = ((0x18 << 24) | (0x93 << 16) | (DALY_PC << 8) | DALY_BMS_ADDR) >>> 0; // 0x18934001
const DALY_ID98     = ((0x18 << 24) | (0x98 << 16) | (DALY_PC << 8) | DALY_BMS_ADDR) >>> 0; // 0x18984001
// ===

export const views = reactive({
    digital: {
        name: 'Digital',
        display: { telemetry: true },
        ch: {
            din1: { name: 'DIN1' },
            din2: { name: 'DIN2' },
            din3: { name: 'DIN3' },
            din4: { name: 'DIN4' }
        }
    },
    analog: {
        name: 'Analog',
        display: { telemetry: true },
        ch: {
            ain1: { name: 'AIN1', divider: false, multiplier: 1, unit: 'Volt' },
            ain2: { name: 'AIN2', divider: false, multiplier: 1, unit: 'Volt' },
            ain3: { name: 'AIN3', divider: false, multiplier: 1, unit: 'Volt' },
            ain4: { name: 'AIN4', divider: false, multiplier: 1, unit: 'Volt' },
            ain5: { name: 'AIN5', multiplier: 1, unit: 'Volt' },
            ain6: { name: 'AIN6', multiplier: 1, unit: 'Volt' },
            ain7: { name: 'AIN7', multiplier: 1, unit: 'Volt' },
            ain8: { name: 'AIN8', multiplier: 1, unit: 'Volt' }
        }
    },
    gyro: {
        name: 'Gyro',
        display: { telemetry: true }
    },
    can: {
        name: 'CAN',
        display: { telemetry: true, viewer: true },
        view: {
            // ── EZkontrol 0x180117EF (ID1) ──────────────────────────────────
            // NOTE: Speed 명세 오프셋 "-32000"은 오타. 테스트 벡터(raw=34000→1400rpm) 기준 -2000 적용.
            'EZ Speed':       { id: EZ_ID1, mode: 'byte', start: 6, end: 7, endian: 'little', multiplier: 0.1, offset: -2000, unit: 'RPM' },
            'EZ Bus Voltage': { id: EZ_ID1, mode: 'byte', start: 0, end: 1, endian: 'little', multiplier: 0.1,              unit: 'Volt' },
            'EZ Bus Current': { id: EZ_ID1, mode: 'byte', start: 2, end: 3, endian: 'little', multiplier: 0.1, offset: -3200, unit: 'Current' },

            // ── EZkontrol 0x180217EF (ID2) ──────────────────────────────────
            'EZ Ctrl Temp':  { id: EZ_ID2, mode: 'byte', start: 0, end: 0, endian: 'little', multiplier: 1, offset: -40, unit: 'Temperature' },
            'EZ Motor Temp': { id: EZ_ID2, mode: 'byte', start: 1, end: 1, endian: 'little', multiplier: 1, offset: -40, unit: 'Temperature' },
            'EZ Accel':      { id: EZ_ID2, mode: 'byte', start: 2, end: 2, endian: 'little', multiplier: 1,              unit: 'Percent' },

            // EZ Error B4 (byte 4 = bit 32~39)
            'EZ Overcurrent':    { id: EZ_ID2, mode: 'bit', start: 32, end: 32, multiplier: 1, unit: 'Flag' },
            'EZ Overload':       { id: EZ_ID2, mode: 'bit', start: 33, end: 33, multiplier: 1, unit: 'Flag' },
            'EZ Overvoltage':    { id: EZ_ID2, mode: 'bit', start: 34, end: 34, multiplier: 1, unit: 'Flag' },
            'EZ Undervoltage':   { id: EZ_ID2, mode: 'bit', start: 35, end: 35, multiplier: 1, unit: 'Flag' },
            'EZ Ctrl Overheat':  { id: EZ_ID2, mode: 'bit', start: 36, end: 36, multiplier: 1, unit: 'Flag' },
            'EZ Motor Overheat': { id: EZ_ID2, mode: 'bit', start: 37, end: 37, multiplier: 1, unit: 'Flag' },
            'EZ Motor Stalled':  { id: EZ_ID2, mode: 'bit', start: 38, end: 38, multiplier: 1, unit: 'Flag' },
            'EZ Out of Phase':   { id: EZ_ID2, mode: 'bit', start: 39, end: 39, multiplier: 1, unit: 'Flag' },

            // EZ Error B5 (byte 5 = bit 40~47)
            'EZ Motor Sensor': { id: EZ_ID2, mode: 'bit', start: 40, end: 40, multiplier: 1, unit: 'Flag' },
            'EZ AUX Sensor':   { id: EZ_ID2, mode: 'bit', start: 41, end: 41, multiplier: 1, unit: 'Flag' },
            'EZ Encoder':      { id: EZ_ID2, mode: 'bit', start: 42, end: 42, multiplier: 1, unit: 'Flag' },
            'EZ Anti-Runaway': { id: EZ_ID2, mode: 'bit', start: 43, end: 43, multiplier: 1, unit: 'Flag' },
            'EZ Main Accel':   { id: EZ_ID2, mode: 'bit', start: 44, end: 44, multiplier: 1, unit: 'Flag' },
            'EZ AUX Accel':    { id: EZ_ID2, mode: 'bit', start: 45, end: 45, multiplier: 1, unit: 'Flag' },
            'EZ Pre-charge':   { id: EZ_ID2, mode: 'bit', start: 46, end: 46, multiplier: 1, unit: 'Flag' },
            'EZ DC Contactor': { id: EZ_ID2, mode: 'bit', start: 47, end: 47, multiplier: 1, unit: 'Flag' },

            // EZ Error B6 (byte 6 = bit 48~53)
            'EZ Power Valve':    { id: EZ_ID2, mode: 'bit', start: 48, end: 48, multiplier: 1, unit: 'Flag' },
            'EZ Current Sensor': { id: EZ_ID2, mode: 'bit', start: 49, end: 49, multiplier: 1, unit: 'Flag' },
            'EZ Auto-tune':      { id: EZ_ID2, mode: 'bit', start: 50, end: 50, multiplier: 1, unit: 'Flag' },
            'EZ RS485':          { id: EZ_ID2, mode: 'bit', start: 51, end: 51, multiplier: 1, unit: 'Flag' },
            'EZ CAN':            { id: EZ_ID2, mode: 'bit', start: 52, end: 52, multiplier: 1, unit: 'Flag' },
            'EZ Software':       { id: EZ_ID2, mode: 'bit', start: 53, end: 53, multiplier: 1, unit: 'Flag' },

            // ── Daly BMS 0x18904001 (0x90) ──────────────────────────────────
            'BMS Voltage': { id: DALY_ID90, mode: 'byte', start: 0, end: 1, endian: 'big', multiplier: 0.1,              unit: 'Volt' },
            'BMS Current': { id: DALY_ID90, mode: 'byte', start: 4, end: 5, endian: 'big', multiplier: 0.1, offset: -3000, unit: 'Current' },
            'BMS SOC':     { id: DALY_ID90, mode: 'byte', start: 6, end: 7, endian: 'big', multiplier: 0.1,              unit: 'Percent' },

            // ── Daly BMS 0x18934001 (0x93) ──────────────────────────────────
            'BMS Remain Cap': { id: DALY_ID93, mode: 'byte', start: 4, end: 7, endian: 'big', multiplier: 1, unit: 'Capacity' },

            // ── Daly BMS 0x18984001 (0x98) 고장 플래그 ──────────────────────
            // B0 (bit 0~7): 셀/총전압 과·저전압
            'BMS Cell OV1': { id: DALY_ID98, mode: 'bit', start: 0, end: 0, multiplier: 1, unit: 'Flag' },
            'BMS Cell OV2': { id: DALY_ID98, mode: 'bit', start: 1, end: 1, multiplier: 1, unit: 'Flag' },
            'BMS Cell UV1': { id: DALY_ID98, mode: 'bit', start: 2, end: 2, multiplier: 1, unit: 'Flag' },
            'BMS Cell UV2': { id: DALY_ID98, mode: 'bit', start: 3, end: 3, multiplier: 1, unit: 'Flag' },
            'BMS Pack OV1': { id: DALY_ID98, mode: 'bit', start: 4, end: 4, multiplier: 1, unit: 'Flag' },
            'BMS Pack OV2': { id: DALY_ID98, mode: 'bit', start: 5, end: 5, multiplier: 1, unit: 'Flag' },
            'BMS Pack UV1': { id: DALY_ID98, mode: 'bit', start: 6, end: 6, multiplier: 1, unit: 'Flag' },
            'BMS Pack UV2': { id: DALY_ID98, mode: 'bit', start: 7, end: 7, multiplier: 1, unit: 'Flag' },

            // B1 (bit 8~15): 충/방전 고·저온
            'BMS Chg Hot1':  { id: DALY_ID98, mode: 'bit', start:  8, end:  8, multiplier: 1, unit: 'Flag' },
            'BMS Chg Hot2':  { id: DALY_ID98, mode: 'bit', start:  9, end:  9, multiplier: 1, unit: 'Flag' },
            'BMS Chg Cold1': { id: DALY_ID98, mode: 'bit', start: 10, end: 10, multiplier: 1, unit: 'Flag' },
            'BMS Chg Cold2': { id: DALY_ID98, mode: 'bit', start: 11, end: 11, multiplier: 1, unit: 'Flag' },
            'BMS Dsg Hot1':  { id: DALY_ID98, mode: 'bit', start: 12, end: 12, multiplier: 1, unit: 'Flag' },
            'BMS Dsg Hot2':  { id: DALY_ID98, mode: 'bit', start: 13, end: 13, multiplier: 1, unit: 'Flag' },
            'BMS Dsg Cold1': { id: DALY_ID98, mode: 'bit', start: 14, end: 14, multiplier: 1, unit: 'Flag' },
            'BMS Dsg Cold2': { id: DALY_ID98, mode: 'bit', start: 15, end: 15, multiplier: 1, unit: 'Flag' },

            // B2 (bit 16~23): 충/방전 과전류, SOC 고·저
            'BMS Chg OC1': { id: DALY_ID98, mode: 'bit', start: 16, end: 16, multiplier: 1, unit: 'Flag' },
            'BMS Chg OC2': { id: DALY_ID98, mode: 'bit', start: 17, end: 17, multiplier: 1, unit: 'Flag' },
            'BMS Dsg OC1': { id: DALY_ID98, mode: 'bit', start: 18, end: 18, multiplier: 1, unit: 'Flag' },
            'BMS Dsg OC2': { id: DALY_ID98, mode: 'bit', start: 19, end: 19, multiplier: 1, unit: 'Flag' },
            'BMS SOC Hi1': { id: DALY_ID98, mode: 'bit', start: 20, end: 20, multiplier: 1, unit: 'Flag' },
            'BMS SOC Hi2': { id: DALY_ID98, mode: 'bit', start: 21, end: 21, multiplier: 1, unit: 'Flag' },
            'BMS SOC Lo1': { id: DALY_ID98, mode: 'bit', start: 22, end: 22, multiplier: 1, unit: 'Flag' },
            'BMS SOC Lo2': { id: DALY_ID98, mode: 'bit', start: 23, end: 23, multiplier: 1, unit: 'Flag' },

            // B3 (bit 24~27): 전압차·온도차
            'BMS VDiff1': { id: DALY_ID98, mode: 'bit', start: 24, end: 24, multiplier: 1, unit: 'Flag' },
            'BMS VDiff2': { id: DALY_ID98, mode: 'bit', start: 25, end: 25, multiplier: 1, unit: 'Flag' },
            'BMS TDiff1': { id: DALY_ID98, mode: 'bit', start: 26, end: 26, multiplier: 1, unit: 'Flag' },
            'BMS TDiff2': { id: DALY_ID98, mode: 'bit', start: 27, end: 27, multiplier: 1, unit: 'Flag' },

            // B4 (bit 32~39): MOS 고온알람·온도센서 오류·접착/개방 오류
            // NOTE: 명세가 bit0~7을 3개 그룹으로만 기술. 아래 할당은 Daly 표준 문서 기반.
            'BMS ChgMOS Hot':  { id: DALY_ID98, mode: 'bit', start: 32, end: 32, multiplier: 1, unit: 'Flag' },
            'BMS DsgMOS Hot':  { id: DALY_ID98, mode: 'bit', start: 33, end: 33, multiplier: 1, unit: 'Flag' },
            'BMS ChgMOS Temp': { id: DALY_ID98, mode: 'bit', start: 34, end: 34, multiplier: 1, unit: 'Flag' },
            'BMS DsgMOS Temp': { id: DALY_ID98, mode: 'bit', start: 35, end: 35, multiplier: 1, unit: 'Flag' },
            'BMS ChgMOS Adh':  { id: DALY_ID98, mode: 'bit', start: 36, end: 36, multiplier: 1, unit: 'Flag' },
            'BMS ChgMOS Open': { id: DALY_ID98, mode: 'bit', start: 37, end: 37, multiplier: 1, unit: 'Flag' },
            'BMS DsgMOS Adh':  { id: DALY_ID98, mode: 'bit', start: 38, end: 38, multiplier: 1, unit: 'Flag' },
            'BMS DsgMOS Open': { id: DALY_ID98, mode: 'bit', start: 39, end: 39, multiplier: 1, unit: 'Flag' },

            // B5 (bit 40~47): AFE·전압수집·셀온도·EEPROM·RTC·프리차지·통신 오류
            'BMS AFE':      { id: DALY_ID98, mode: 'bit', start: 40, end: 40, multiplier: 1, unit: 'Flag' },
            'BMS VCollect': { id: DALY_ID98, mode: 'bit', start: 41, end: 41, multiplier: 1, unit: 'Flag' },
            'BMS CellTemp': { id: DALY_ID98, mode: 'bit', start: 42, end: 42, multiplier: 1, unit: 'Flag' },
            'BMS EEPROM':   { id: DALY_ID98, mode: 'bit', start: 43, end: 43, multiplier: 1, unit: 'Flag' },
            'BMS RTC':      { id: DALY_ID98, mode: 'bit', start: 44, end: 44, multiplier: 1, unit: 'Flag' },
            'BMS PreCharge':{ id: DALY_ID98, mode: 'bit', start: 45, end: 45, multiplier: 1, unit: 'Flag' },
            'BMS Comm':     { id: DALY_ID98, mode: 'bit', start: 46, end: 46, multiplier: 1, unit: 'Flag' },
            'BMS IntComm':  { id: DALY_ID98, mode: 'bit', start: 47, end: 47, multiplier: 1, unit: 'Flag' },

            // B6 (bit 48~51): 전류모듈·총전압검출·단락보호·저전압충전금지
            'BMS CurrMod': { id: DALY_ID98, mode: 'bit', start: 48, end: 48, multiplier: 1, unit: 'Flag' },
            'BMS VDetect': { id: DALY_ID98, mode: 'bit', start: 49, end: 49, multiplier: 1, unit: 'Flag' },
            'BMS Short':   { id: DALY_ID98, mode: 'bit', start: 50, end: 50, multiplier: 1, unit: 'Flag' },
            'BMS LowChg':  { id: DALY_ID98, mode: 'bit', start: 51, end: 51, multiplier: 1, unit: 'Flag' },

            // B7: Fault Code (raw byte 값)
            'BMS Fault Code': { id: DALY_ID98, mode: 'byte', start: 7, end: 7, endian: 'big', multiplier: 1, unit: 'Flag' }
        }
    },
    gps: {
        name: 'GPS',
        display: { telemetry: true, viewer: true }
    }
});

export const units = reactive({
    Volt: { unit: 'V', display: 'Volt (V)', default: true },
    Temperature: { unit: '°C', display: 'Temperature (°C)', default: true },
    Acceleration: { unit: 'g', display: 'Acceleration (g)', default: true },
    'Angular Velocity': { unit: '°/s', display: 'Angular Velocity (°/s)', default: true },
    Speed: { unit: 'km/h', display: 'Speed (km/h)', default: true },
    RPM: { unit: 'rpm', display: 'RPM (rpm)', default: true },
    Current: { unit: 'A', display: 'Current (A)', default: true },
    Percent: { unit: '%', display: 'Percent (%)', default: true },
    Capacity: { unit: 'mAh', display: 'Capacity (mAh)', default: true },
    Flag: { unit: '', display: 'Flag', default: false }
});

export const defaults = {
    views: JSON.parse(JSON.stringify(views)),
    units: JSON.parse(JSON.stringify(units))
};

export const can_decoder = reactive({});

function load_decoder() {
    // First pass: group decoders by CAN ID
    Object.entries(views.can.view).forEach(([k, v]) => {
        if (!can_decoder[v.id]) {
            can_decoder[v.id] = [];
        }

        const entry = { name: k, ...v };
        entry.offset = v.offset ?? 0;

        if (v.filter && v.mask) {
            entry._filter = parse_hex_bytes(v.filter);
            entry._mask = parse_hex_bytes(v.mask);
        }

        can_decoder[v.id].push(entry);
    });

    // Second pass: assign idx in can_decoder iteration order
    // Object.entries() sorts integer keys numerically, which is the same
    // order used when building chart series in telemetry.vue and viewer.vue
    let idx = 1;
    for (const decoders of Object.values(can_decoder)) {
        for (const decoder of decoders) {
            decoder.idx = idx++;
        }
    }
}

let save_timer = null;

function debounced_save() {
    clearTimeout(save_timer);
    save_timer = setTimeout(save_view, 500);
}

watch(views, debounced_save, { deep: true });
watch(units, debounced_save, { deep: true });

load_view();
load_decoder();

export function save_view() {
    localStorage.setItem('views', JSON.stringify(views));
    localStorage.setItem('units', JSON.stringify(units));
}

export function load_view() {
    const v = localStorage.getItem('views');
    const u = localStorage.getItem('units');

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

    if (u) {
        const parsed = JSON.parse(u);

        for (const key in parsed) {
            if (units[key]) {
                Object.assign(units[key], parsed[key]);
            } else {
                units[key] = parsed[key];
            }
        }
    }
}

const color_cnt = 50;

export const colors = Array.from({ length: color_cnt }, (_, i) => {
    const hue = (i * 137.508) % 360;
    const saturation = 70;
    const lightness = 50;
    const [r, g, b] = hsl_to_rgb(hue, saturation, lightness);
    return rgb_to_hex(r, g, b);
});

function hsl_to_rgb(h, s, l) {
    s /= 100;
    l /= 100;
    const k = (n) => (n + h / 30) % 12;
    const a = s * Math.min(l, 1 - l);
    const f = (n) => l - a * Math.max(-1, Math.min(k(n) - 3, Math.min(9 - k(n), 1)));
    return [Math.round(255 * f(0)), Math.round(255 * f(8)), Math.round(255 * f(4))];
}

function rgb_to_hex(r, g, b) {
    return '#' + [r, g, b].map((x) => x.toString(16).padStart(2, '0')).join('');
}
