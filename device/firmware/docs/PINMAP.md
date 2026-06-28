# ESP32-S3 배선 체크리스트 (Monolith v2 펌웨어)

- **보드:** ESP32-S3-DevKitC-1
- **기준:** `device/firmware/main/` 소스 코드 (코드 대조 검증 2026-06-27)
- **이 문서는 "손으로 직접 배선해야 하는 핀"만 담은 체크리스트입니다.** 보드 온보드 부품, 칩 내부 자동 동작, ESP-IDF 기본 점유(USB로만 접근) 항목은 메인 표에서 제외하고 맨 아래 [배선 불필요(참고)](#배선-불필요-참고)에 따로 모았습니다.
- 변종 표기: `Original`/`Mini`는 `CONFIG_MONOLITH_MINI` 컴파일 플래그로 갈리는 핀. 이 체크리스트는 Original(전체 센서) 기준입니다.

## 배선 체크리스트 (GPIO 번호순)

| GPIO | 인터페이스 | 신호 | 연결 대상 | 변종 | 정의 위치 |
|:----:|-----------|------|-----------|:----:|-----------|
| 9  | I2C0 SDA        | SDA       | RTC·자이로·LCD 공유 버스   | 공통 | `main/main.c:306` |
| 10 | I2C0 SCL        | SCL       | RTC·자이로·LCD 공유 버스   | 공통 | `main/main.c:305` |
| 11 | GPIO In (ISR)   | DIN1      | 디지털 입력 1 (휠스피드 등) | Original | `main/peripheral/digital.c:19,36` |
| 12 | GPIO In (ISR)   | DIN2      | 디지털 입력 2             | Original | `main/peripheral/digital.c:19,37` |
| 13 | GPIO In (ISR)   | DIN3      | 디지털 입력 3             | Original | `main/peripheral/digital.c:19,38` |
| 14 | GPIO In (ISR)   | DIN4      | 디지털 입력 4             | Original | `main/peripheral/digital.c:19,39` |
| 15 | TWAI (CAN) TX   | CAN TX    | CAN 트랜시버              | 공통 | `main/peripheral/can.c:89` |
| 16 | TWAI (CAN) RX   | CAN RX    | CAN 트랜시버              | 공통 | `main/peripheral/can.c:89` |
| 17 | UART1 TX        | → GPS RX  | u-blox GPS                | 공통 | `main/peripheral/gps.c:60` |
| 18 | UART1 RX        | ← GPS TX  | u-blox GPS                | 공통 | `main/peripheral/gps.c:60` |
| 39 | SPI2 SCK        | SD SCK    | SD 카드 모듈 (SDSPI)      | 공통 | `main/peripheral/sdcard.c:13` |
| 40 | SPI2 MOSI       | SD MOSI   | SD 카드 모듈 (SDSPI)      | 공통 | `main/peripheral/sdcard.c:14` |
| 41 | SPI2 CS         | SD CS     | SD 카드 모듈 (SDSPI)      | 공통 | `main/peripheral/sdcard.c:16` |
| 42 | I2C1 SCL        | SCL       | ADS1115 모듈 ×2 (헤더 SCL) | Original | `main/peripheral/analog.c:69` |
| 47 | I2C1 SDA        | SDA       | ADS1115 모듈 ×2 (헤더 SDA) | Original | `main/peripheral/analog.c:70` |
| 48 | SPI2 MISO       | SD MISO   | SD 카드 모듈 (SDSPI)      | 공통 | `main/peripheral/sdcard.c:15` |

---

## 인터페이스별 결선

### I2C

| 버스 | SDA | SCL | 디바이스 (주소) | 변종 | 정의 위치 |
|------|:---:|:---:|-----------------|:----:|-----------|
| `I2C_NUM_0` | GPIO9 | GPIO10 | RTC `0x51`, MPU-6050 자이로 `0x68`, PCF8574 LCD `0x27` | 공통 | `main/main.c:305-306` |
| `I2C_NUM_1` | GPIO47 | GPIO42 | ADS1115 모듈 `0x48`, ADS1115 모듈 `0x49` | Original | `main/peripheral/analog.c:69-70` |

- I2C0는 `rtc_init()`에서 1회 초기화하고, 자이로/디스플레이 태스크는 `i2c_master_get_bus_handle()`로 핸들을 공유합니다.
- I2C1은 Mini 변종에서는 사용되지 않습니다 (`#ifndef CONFIG_MONOLITH_MINI`).

#### ADS1115 파란색 모듈보드 ×2 (I2C1 버스 공유)

I2C는 버스라 두 모듈이 SDA/SCL을 공유하고, **ADDR 핀 결선으로 주소만 분리**합니다. 모듈 헤더 → ESP32 결선:

| 모듈 헤더 | 연결 | 비고 |
|-----------|------|------|
| VDD  | **3.3V** | ★ 5V 금지 — 온보드 풀업이 I2C 라인을 5V로 끌어올려 ESP32-S3 손상 |
| GND  | GND | |
| SCL  | GPIO42 | 두 모듈 공통 |
| SDA  | GPIO47 | 두 모듈 공통 |
| ADDR | 모듈1→GND / 모듈2→VDD | 주소 분리 (아래) |
| ALRT | **미연결** | ALERT/RDY 펌웨어 미사용 |
| A0~A3 | 측정 대상 입력 | 채널 매핑은 아래 |

| 모듈 | I2C 주소 | ADDR | A0 | A1 | A2 | A3 |
|------|:--------:|------|----|----|----|----|
| 모듈1 (adc1) | `0x48` | →GND | ain1 | ain2 | ain3 | ain4 |
| 모듈2 (adc2) | `0x49` | →VDD | ain5 | ain6 | ain7 | ain8 |

- 모듈도 동일 ADS1115 칩이라 펌웨어 로직(레지스터·MUX·±4.096V FSR·860SPS 변환)은 베어 IC와 동일하며 코드 변경 없음. 정의: `main/peripheral/analog.c:23-34,83,89`.
- 선형 포텐쇼미터 등 아날로그 센서는 모듈 헤더 A0~A3에 물립니다.
- 두 모듈의 온보드 10k 풀업이 병렬(≈5k)로 걸립니다. 기존 보드에 베어 IC용 외부 풀업이 따로 있었다면 풀업이 과해지므로 한쪽을 제거하는 게 안전합니다(하드웨어 확인 사항).

### SPI (SPI2_HOST) — SD 카드 모듈

| 신호 | GPIO | 정의 위치 |
|------|:----:|-----------|
| SCK  | GPIO39 | `main/peripheral/sdcard.c:13` |
| MOSI | GPIO40 | `main/peripheral/sdcard.c:14` |
| MISO | GPIO48 | `main/peripheral/sdcard.c:15` |
| CS   | GPIO41 | `main/peripheral/sdcard.c:16` |

- SD 카드는 SDMMC가 아닌 **SDSPI** 모드, 프로빙 속도 400kHz.

### UART — GPS

| 포트 | TX | RX | 대상 | 정의 위치 |
|------|:--:|:--:|------|-----------|
| `UART_NUM_1` | GPIO17 | GPIO18 | u-blox GPS (보율 자동탐지) | `main/peripheral/gps.c:60` |

### CAN (TWAI)

| 신호 | GPIO | 정의 위치 |
|------|:----:|-----------|
| TX | GPIO15 | `main/peripheral/can.c:89` |
| RX | GPIO16 | `main/peripheral/can.c:89` |

### 디지털 입력 (Original 전용)

| GPIO | 신호 | 모드 | 정의 위치 |
|:----:|------|------|-----------|
| 11–14 | DIN1–4 | Input + ISR (pull-down) | `main/peripheral/digital.c:19,36-39` |

- 휠스피드센서 등 펄스/온오프 입력이 여기에 연결됩니다. `CONFIG_MONOLITH_MINI`에서는 사용 안 함.

---

## 배선 불필요 (참고)

아래 항목은 **외부 배선이 필요 없어** 메인 체크리스트에서 제외했습니다. 코드에는 그대로 남아 있습니다.

| GPIO | 항목 | 분류 | 이유 | 정의 위치 |
|:----:|------|------|------|-----------|
| 5  | 상태 LED        | 표시등(옵션) | 외부 LED 미배선해도 펌웨어 동작 (상태는 MQTT 시스로그로도 전송) | `main/main.c:109,137` |
| 21 | 설정 리셋 버튼  | 온보드        | 보드 버튼 사용, 외부 배선 불필요 (3초 길게 눌러 NVS 초기화) | `main/main.c:83,148-155` |
| 38 | 온보드 RGB LED  | 온보드        | DevKitC-1 v1.1 보드 내장, 배선 대상 아님 (부팅 시 끄기만 함) | `main/main.c:121-134` |
| 8  | 배터리전압 ADC  | Mini 전용     | Original 빌드에는 컴파일되지 않음 (Mini에서만 ADC1_CH7로 내부 전압 측정) | `main/peripheral/analog.c:181-213` |
| —  | 내부 온도센서   | Mini 전용     | Original에서는 측정 코드 제거됨 — ain8 슬롯을 ADS1115 A3로 대체. Mini에서만 ain8에 기록 | `main/peripheral/analog.c` |
| 43/44 | UART0 콘솔   | ESP-IDF 기본  | 콘솔/모니터용. USB로 접근, 따로 배선 안 함 | ESP-IDF 기본 |
| 19/20 | USB         | ESP-IDF 기본  | USB 케이블로만 사용 | ESP-IDF 기본 |

---

## 핀 중복·충돌 / 여유 핀

- **배선 대상 핀(9–18, 39–42, 47, 48) 간 GPIO 중복 없음.** 모두 고유합니다.
- **GPIO48 주의:** DevKitC-1 **v1.0** 에서는 GPIO48이 온보드 WS2812B였습니다. v1.1에서는 SD MISO로 무충돌이나, 실물 보드가 v1.0이면 SD MISO와 충돌하니 리비전 확인 필요. (커밋 `1734a02`에서 I2C1 SCL을 GPIO48→42로 옮겨 충돌 해소.)
- **여유 핀:** GPIO45/46(스트래핑, 미사용). 스트래핑 핀 GPIO0/3도 미사용이라 부팅 안전. 배선 불필요 처리된 GPIO5/8/38도 외부 결선은 비어 있습니다(온보드/변종 점유).
