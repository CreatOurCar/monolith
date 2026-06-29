# ESP32-S3 GPIO 사용 현황 (Monolith v2 — Original)

- **보드/모듈:** ESP32-S3-DevKitC-1 **v1.1** + ESP32-S3-WROOM-1 **N16R8** (16MB Flash + 8MB Octal PSRAM)
- **변종:** Original (`CONFIG_MONOLITH_MINI` 미정의) 기준
- **온보드 RGB LED:** GPIO38 (v1.1 기준. 공식 문서 *"Addressable RGB LED, driven by GPIO38"*)
- **분류 범례:** `사용중` = 현재 펌웨어가 점유 / `자유(안전)` = 제약 없이 사용 가능 / `자유(조건부)` = 스트래핑·USB·콘솔 등 주의 후 사용 / `예약·금지` = 칩에 없거나 내장 플래시·PSRAM
- 코드 근거는 `main/` 소스 기준. 칩 제약은 ESP32-S3 / WROOM-1 N16R8 datasheet 기준.

| GPIO | 기능 | 분류 | 비고 | 코드 근거 |
|:----:|------|:----:|------|-----------|
| 0  | 부팅 스트래핑 (BOOT_SEL) | 자유(조건부) | LOW=다운로드/HIGH=일반부팅, 내부 약풀업. 부팅 후 런타임 사용 가능, 부팅 중 LOW 인가 금지 | — |
| 1  | 미점유 | 자유(안전) | ADC1_CH0 (아날로그 확장 가능) | — |
| 2  | 미점유 | 자유(안전) | ADC1_CH1 | — |
| 3  | JTAG 스트래핑 (JTAG_SEL) | 자유(조건부) | IDF 기본 JTAG 활성 시 점유, 비활성화 후 사용 가능 | — |
| 4  | 미점유 | 자유(안전) | ADC1_CH3 | — |
| 5  | 상태 LED (OUTPUT_OD) | 사용중 | 외부 LED 미배선해도 task_led 동작 | `main/main.c:109,137` |
| 6  | 미점유 | 자유(안전) | ADC1_CH5 | — |
| 7  | 미점유 | 자유(안전) | ADC1_CH6 | — |
| 8  | 미점유 (Original) | 자유(안전) | ADC1_CH7. Mini 전용 배터리전압 ADC → Original 자유 | `main/peripheral/analog.c:172` (Mini만) |
| 9  | I2C0 SDA — RTC·자이로·LCD | 사용중 | 공유 버스 (RTC 0x51, MPU6050 0x68, PCF8574 0x27) | `main/main.c:306` |
| 10 | I2C0 SCL — RTC·자이로·LCD | 사용중 | 공유 버스 | `main/main.c:305` |
| 11 | DIN1 디지털 입력 (ISR) | 사용중 | pull-down + ANYEDGE | `main/peripheral/digital.c:19,36` |
| 12 | DIN2 디지털 입력 (ISR) | 사용중 | | `main/peripheral/digital.c:19,37` |
| 13 | DIN3 디지털 입력 (ISR) | 사용중 | | `main/peripheral/digital.c:19,38` |
| 14 | DIN4 디지털 입력 (ISR) | 사용중 | | `main/peripheral/digital.c:19,39` |
| 15 | TWAI TX (CAN) | 사용중 | CAN 트랜시버 | `main/peripheral/can.c:89` |
| 16 | TWAI RX (CAN) | 사용중 | CAN 트랜시버 | `main/peripheral/can.c:89` |
| 17 | UART1 TX → GPS RX | 사용중 | u-blox GPS | `main/peripheral/gps.c:60` |
| 18 | UART1 RX ← GPS TX | 사용중 | u-blox GPS | `main/peripheral/gps.c:60` |
| 19 | USB D− (Serial/JTAG) | 자유(조건부) | 칩 내장 USB PHY. 플래싱·JTAG 사용 시 필수 점유, USB 미사용 PCB만 GPIO 전용 가능 | — |
| 20 | USB D+ (Serial/JTAG) | 자유(조건부) | 위와 동일 | — |
| 21 | 설정 리셋 버튼 (ISR) | 사용중 | INPUT + pull-down, 3초 길게 눌러 NVS 초기화 | `main/main.c:83,148,155` |
| 22 | 칩에 없음 | 예약·금지 | ESP32-S3 GPIO 범위: 0–21, 26–48 | — |
| 23 | 칩에 없음 | 예약·금지 | | — |
| 24 | 칩에 없음 | 예약·금지 | | — |
| 25 | 칩에 없음 | 예약·금지 | | — |
| 26 | 내장 SPI 플래시 | 예약·금지 | WROOM-1 모듈 내부 배선, 외부 미노출 | — |
| 27 | 내장 SPI 플래시 | 예약·금지 | | — |
| 28 | 내장 SPI 플래시 | 예약·금지 | | — |
| 29 | 내장 SPI 플래시 | 예약·금지 | | — |
| 30 | 내장 SPI 플래시 | 예약·금지 | | — |
| 31 | 내장 SPI 플래시 | 예약·금지 | | — |
| 32 | 내장 SPI 플래시 | 예약·금지 | | — |
| 33 | Octal PSRAM (N16R8) | 예약·금지 | `CONFIG_SPIRAM` 꺼져도 물리 배선 존재 → 외부 사용 금지 | — |
| 34 | Octal PSRAM (N16R8) | 예약·금지 | | — |
| 35 | Octal PSRAM (N16R8) | 예약·금지 | | — |
| 36 | Octal PSRAM (N16R8) | 예약·금지 | | — |
| 37 | Octal PSRAM (N16R8) | 예약·금지 | | — |
| 38 | WS2812B 온보드 RGB LED (led_strip RMT, 부팅 시 소등) | 사용중 | DevKit v1.1 온보드 LED. 베어 모듈 PCB엔 LED 없으나 펌웨어가 RMT로 점유 → 외부 신호 금지 권장 | `main/main.c:121-134` |
| 39 | SD SPI SCK | 사용중 | SDSPI, 400kHz 프로빙 | `main/peripheral/sdcard.c:13` |
| 40 | SD SPI MOSI | 사용중 | | `main/peripheral/sdcard.c:14` |
| 41 | SD SPI CS | 사용중 | | `main/peripheral/sdcard.c:16` |
| 42 | I2C1 SCL — ADS1115 ×2 | 사용중 | 모듈 0x48 / 0x49 | `main/peripheral/analog.c:61` |
| 43 | UART0 TX (콘솔 기본) | 자유(조건부) | 코드 명시 배정 없음(IDF 자동). 콘솔 미사용·재배정 시 자유 | — |
| 44 | UART0 RX (콘솔 기본) | 자유(조건부) | 위와 동일 | — |
| 45 | 전압 스트래핑 (VDD_SPI) | 자유(조건부) | LOW=1.8V/HIGH=3.3V SPI 플래시 전압. 부팅 후 런타임 사용 가능, 외부 풀다운 금지 | — |
| 46 | ROM 메시지 스트래핑 | 자유(조건부) | LOW=억제/HIGH=출력. 부팅 후 런타임 사용 가능 (내부 약풀다운 — 추측) | — |
| 47 | I2C1 SDA — ADS1115 ×2 | 사용중 | | `main/peripheral/analog.c:62` |
| 48 | SD SPI MISO | 사용중 | v1.0에선 온보드 LED였으나 v1.1에선 자유 핀 → SD MISO로 정상 사용 | `main/peripheral/sdcard.c:15` |

---

## 요약

- **제약 없이 바로 쓸 수 있는 자유 핀:** `GPIO 1, 2, 4, 6, 7, 8` (모두 ADC1 채널)
- **조건부 자유 핀 (주의 후 사용):** `0, 3` (스트래핑) · `19, 20` (USB) · `43, 44` (UART0 콘솔) · `45, 46` (스트래핑)
- **외부 신호 배선 금지:** `22–25` (칩에 없음) · `26–32` (내장 플래시) · `33–37` (N16R8 Octal PSRAM) · `38` (온보드 LED, DevKit 호환 위해 회피 권장)
- 펌웨어 무수정 전제 시 `사용중` 기능은 PCB에서 해당 GPIO로 그대로 배선해야 함.
