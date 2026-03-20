# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Monolith v2 is a DIY wireless data logging platform for Student Formula / Baja racing. It consists of three independent subsystems: ESP32-S3 firmware (C), a Vue 3 web control hub, and KiCAD hardware designs. Two hardware variants exist: Original (full sensor suite) and Mini (compact, CAN + GPS + IMU only), controlled by the `CONFIG_MONOLITH_MINI` compile flag.

## Build & Development Commands

### Web Frontend (`web/`)
```bash
npm install          # install dependencies
npm run dev          # dev server (HTTPS, port 5173)
npm run build        # production build (also builds docs into dist/docs)
npm run lint         # ESLint with auto-fix
npm run preview      # preview production build
```

### Firmware (`device/firmware/`)
Requires ESP-IDF v5.5.1 installed at `~/esp/esp-idf`.
```bash
make build           # compile firmware (Original)
make build mini      # compile firmware (Mini variant)
make run             # build, flash (921600 baud), and monitor
make config          # open ESP-IDF menuconfig
make clean           # remove build directory
make dump            # disassemble ELF binary
```

### Server (`server/`)
Docker Compose stack: Traefik (reverse proxy + TLS), Mosquitto (MQTT broker with WebSocket), Nginx (serves web SPA). Requires `.env` file (see `.env.example`).

## Architecture

### Firmware (`device/firmware/main/`)
- **main.c / main.h** — Initialization sequence, system state machine (bitmask-based error/fatal tracking per component), shared globals, binary log protocol definitions
- **peripheral/** — Sensor drivers: `can.c`, `gps.c`, `analog.c`, `digital.c`, `gyroscope.c`, `sdcard.c`
- **network/** — `network.c` (WiFi), `mqtt.c` (telemetry publish), `webserver.c` (local config HTTP server)

Key patterns:
- Per-sensor sampling rates with explicit intervals: Gyro 100Hz (`TASK_INTERVAL_GYRO`, 10ms), Analog 50Hz/10Hz (`TASK_INTERVAL_ANALOG`, 20ms Original / 100ms Mini), CAN ~1kHz (1ms polling), GPS 10Hz (event-driven, hardware limited)
- FreeRTOS tick rate 1000Hz (1ms resolution), LWIP/WiFi pinned to Core 0, sensor tasks NO_AFFINITY
- Binary log protocol: 24-byte fixed records (`log_t`) with magic byte `0xAE`, XOR-folded checksum, millisecond timestamps, 16-byte payload union
- State management via `SET_ERROR`/`SET_FATAL`/`CLEAR_*` macros that use bitmask positions per component
- Configuration stored in NVS (`nvs_storage_t` struct)
- SD fsync: hybrid write-count (512) + time-based (3s) to reduce blocking

### Web Frontend (`web/src/`)
- **views/** — 4 pages: `telemetry.vue` (real-time dashboard), `viewer.vue` (offline log analysis), `device.vue` (device config), `ui.vue` (UI config)
- **service/** — Business logic layer: `mqtt.js` (MQTT client), `protocol.js` (binary log parser matching firmware format), `state.js` (reactive state), `telemetry.js` (data aggregation), `ui.js` (chart config), `map.js` (Kakao Maps), `uplot.js` (chart rendering), `terminal.js` (xterm.js integration)
- **layout/** — App shell (sidebar + topbar + footer)
- **router/** — Vue Router with lazy-loaded views under `AppLayout`

Key patterns:
- PrimeVue components are auto-imported via `unplugin-vue-components`
- `@` alias resolves to `web/src/`
- The web app and firmware share the same binary protocol — `protocol.js` constants must stay in sync with `main.h` definitions

## Code Style

### Web
- Vue SFC tag order: `<script>`, `<template>`, `<style>` (enforced by ESLint)
- Prettier: 4-space indent, single quotes, no trailing commas, 250 char print width, semicolons
- Vue 3 Composition API

### Firmware
- C with ESP-IDF conventions, `.clang-format` present
- Uppercase macros for logging/state utilities (`LOG`, `SYSLOG`, `INFO`, `SET_ERROR`, etc.)

## CI/CD

- **firmware.yml** — Builds both Original and Mini on pushes to `device/firmware/**`
- **release.yml** — Triggered on `v*` tags; packages firmware binaries + PCB gerbers into GitHub releases
- **pcb.yml** — Exports KiCAD gerbers and BOM/CPL for PCB assembly

## Commit Messages

Use lowercase prefixed format: `fix:`, `add:`, `update:`, `bump:`, `impl:` (e.g., `fix: CAN decoder idx mismatch with chart series order`).
