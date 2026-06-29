# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Monolith v2 is a DIY wireless data logging platform for Student Formula / Baja racing. It consists of two independent subsystems: ESP32-S3 firmware (C) and a Vue 3 web control hub. Two hardware variants exist: Original (full sensor suite) and Mini (compact, CAN + GPS + IMU + internal temp/voltage, no digital I/O), controlled by the `CONFIG_MONOLITH_MINI` compile flag.

## Build & Development Commands

### Web Frontend (`web/`)
```bash
npm run dev          # dev server (HTTPS, port 5173)
npm run build        # production build (also builds docs into dist/docs)
npm run lint         # ESLint with auto-fix
```

### Firmware (`device/firmware/`)
Requires **ESP-IDF v6.0.1** (`idf.py --version` to verify — mismatched versions will break the build).
- Linux/macOS: install at `~/esp/esp-idf`
- Windows: install at `C:\esp\v6.0.1\esp-idf`, build via PowerShell (`export.ps1` + `idf.py`) — `make` and Git Bash are not supported on Windows

```bash
make build           # compile firmware (Original)
make build mini      # compile firmware (Mini variant)
make run             # build, flash (921600 baud), and monitor
make config          # open ESP-IDF menuconfig
make clean           # remove build directory
```

### Server (`server/`)
Docker Compose stack. Requires `.env` file (see `.env.example`).

## Key Patterns

- The web app and firmware share the same binary log protocol — `protocol.js` constants must stay in sync with `main/include/protocol.h` definitions
- MQTT telemetry uses batching: CAN (up to 128 records) and syslog (up to 32 records) per single MQTT publish; web frontend parses in 24-byte (`log_t`) chunks
- Core affinity: WiFi/LWIP pinned to Core 0, MQTT publisher pinned to Core 1, ESP-MQTT internal task on Core 0, sensor tasks NO_AFFINITY

## Hardware Notes

- **Board/module:** ESP32-S3-DevKitC-1 **v1.1** + ESP32-S3-WROOM-1 **N16R8** (16MB flash + 8MB octal PSRAM).
- **Board revision matters — always verify pin info against v1.1.** The onboard addressable RGB LED is on **GPIO38** on v1.1 (it was GPIO48 on v1.0). Citing v1.0 data caused a real pin conflict: SD MISO was assigned to GPIO38 and collided with the onboard LED (the symptom — the LED lighting when MISO toggled — itself proves the board is v1.1). Never cite v1.0 pin data. SD MISO is now **GPIO48** (free on v1.1) and must not be moved back to GPIO38.
- **Pins not usable as external I/O:** internal flash **GPIO26–32**, octal PSRAM **GPIO33–37** (reserved by the N16R8 module even when `CONFIG_SPIRAM` is disabled), USB-JTAG **GPIO19/20**, strapping **GPIO0/3/45/46**. GPIO22–25 do not exist on the chip.
- **SD breakout module power:** VCC = **5V** (onboard AMS1117 + level shifter); ESP32-S3 signal lines stay 3.3V — never drive a GPIO with 5V. This was a *separate* issue from the LED pin conflict above.

## CI/CD

- **firmware.yml** — Builds both Original and Mini on pushes to `device/firmware/**`
- **release.yml** — Triggered on `v*` tags; packages firmware binaries into GitHub releases

## Commit Messages

Use lowercase prefixed format: `fix:`, `add:`, `update:`, `bump:`, `impl:` (e.g., `fix: CAN decoder idx mismatch with chart series order`).
