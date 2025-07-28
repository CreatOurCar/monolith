const PROTOCOL_VERSION = 1;
const LOG_MAGIC = 0xAE;
const LOG_SIZE = 24;

const LOG_TYPE = [
  "INVALID",
  "BOOT",
  "SYSTEM",
  "USER_EVENT",
  "CAN",
  "GPS",
  "ANALOG",
  "DIGITAL",
  "GYROSCOPE",
];

const LOG_POS = {
  MAGIC: 0,
  TYPE: 1,
  CHECKSUM: 2,
  TIMESTAMP: 4,
  BOOT: {
    PROTOCOL_VERSION: 8,
    _RESERVED: 9,
    MAC: 10,
    BOOT_TIME: 16,
  },
  CAN: {
    ID: 8,
    EXTENDED: 12,
    REMOTE: 13,
    LEN: 14,
    _RESERVED: 15,
    DATA: 16,
  },
  GPS: {
    LATITUDE: 8,
    LONGITUDE: 12,
    LAT_DIR: 16,
    LON_DIR: 17,
    _RESERVED: 18,
    SPEED: 20,
    COURSE: 22,
  },
  ANALOG: {
    AIN1: 8,
    AIN2: 10,
    AIN3: 12,
    AIN4: 14,
    AIN5: 16,
    AIN6: 18,
    VOLTAGE: 20,
    TEMPERATURE: 22,
  },
  DIGITAL: {
    DIN1: 8,
    DIN2: 12,
    DIN3: 16,
    DIN4: 20,
  },
  GYRO: {
    ACCEL_X: 8,
    ACCEL_Y: 10,
    ACCEL_Z: 12,
    TEMPERATURE: 14,
    GYRO_X: 16,
    GYRO_Y: 18,
    GYRO_Z: 20,
    _RESERVED: 22,
  },
  SYS: {
    MSG: 8,
  },
  USER: {
    MSG: 8,
  },
};

const NVS_POS = {
  WIFI: {
    MAC: 0,
    MACADDR: 6,
    SSID: 24,
    PASSWD: 56,
  },
  DEVICE: {
    SERVER: 88,
    NAME: 152,
    KEY: 184,
    TZ: 216,
  },
  EN: {
    CAN: 256,
    GPS: 257,
    ANALOG: 258,
    DIGITAL: 259,
  },
  CAN: {
    BPS: 260,
    FILTER: 261,
    MASK: 265,
  },
  GPS: {
    DEV: 269,
  },
};

const LOGBUF_POS = {
  STATE: 0,
  GPS: 4,
  GYRO: 28,
  ANALOG: 52,
  DIGITAL: 76,
};

export function parse_cfg(buf) {
  const cfg = {
    wifi: {
      mac: to_string(buf, NVS_POS.WIFI.MACADDR, NVS_POS.WIFI.SSID),
      ssid: to_string(buf, NVS_POS.WIFI.SSID, NVS_POS.WIFI.PASSWD),
      passwd: to_string(buf, NVS_POS.WIFI.PASSWD, NVS_POS.DEVICE.SERVER),
    },
    device: {
      server: to_string(buf, NVS_POS.DEVICE.SERVER, NVS_POS.DEVICE.NAME),
      name: to_string(buf, NVS_POS.DEVICE.NAME, NVS_POS.DEVICE.KEY),
      key: to_string(buf, NVS_POS.DEVICE.KEY, NVS_POS.DEVICE.TZ),
      tz: to_string(buf, NVS_POS.DEVICE.TZ, NVS_POS.EN.CAN),
    },
    en: {
      can: to_uint(8, buf, NVS_POS.EN.CAN),
      gps: to_uint(8, buf, NVS_POS.EN.GPS),
      analog: to_uint(8, buf, NVS_POS.EN.ANALOG),
      digital: to_uint(8, buf, NVS_POS.EN.DIGITAL),
    },
    can: {
      bps: to_uint(8, buf, NVS_POS.CAN.BPS),
      filter: to_uint(32, buf, NVS_POS.CAN.FILTER),
      mask: to_uint(32, buf, NVS_POS.CAN.MASK),
    },
    gps: {
      dev: to_uint(8, buf, NVS_POS.GPS.DEV),
    },
  };

  return cfg;
}

export function validate_checksum(buf) {
  const original = to_uint(16, buf, LOG_POS.CHECKSUM);
  buf[LOG_POS.CHECKSUM] = 0;
  buf[LOG_POS.CHECKSUM + 1] = 0;

  let checksum = 0;

  for (let i = 0; i < LOG_SIZE; i += 4) {
    checksum ^= to_uint(32, buf, i);
  }

  checksum = (checksum & 0xFFFF) + (checksum >> 16);
  return checksum === original;
}

function to_string(buffer, start, end) {
  let str = String.fromCharCode(...buffer.slice(start, end));
  return str.slice(0, str.indexOf('\u0000')); // drop from the null character
}

function to_uint(bit, buffer, start) {
  if (bit <= 0 || bit & (bit - 1) !== 0) {
    throw new Error("Invalid bit count: bit must be a power of two");
  }

  let ret = 0;

  for (let i = 0; i < bit / 8; i++) {
    ret += buffer[start + i] * Math.pow(2, i * 8); // little endian
  }

  return ret;
}

function to_int(bit, buffer, start) {
  return signed(to_uint(bit, buffer, start), bit);
}

function signed(value, bit) {
  return (value > Math.pow(2, bit - 1) - 1) ? value - Math.pow(2, bit) : value;
}
