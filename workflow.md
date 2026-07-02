# Monolith v2 — 펌웨어 워크플로우 지도

이 문서는 레포 구조와 코드 흐름을 훑어보기 위한 지도입니다. web 앱 / 중계 서버 / MQTT / WiFi / RTC가 모두 제거되고, **ESP32-S3 펌웨어 하나가 센서를 읽어 SD카드에 `.log` 파일로 기록하는 단일 시스템**으로 정리된 이후 기준입니다. 기록된 로그는 이 레포가 아니라 외부 업스트림 사이트(https://v2.monolith.luftaquila.io/)에서 분석합니다.

---

# 1. 레포 전체 지도 (어디에 뭐가 있나)

| 폴더 | 역할 | 언어/스택 |
|------|------|-----------|
| `device/firmware` | ESP32-S3 펌웨어 (센서 수집 → SD 기록) | C / ESP-IDF v6.0.1 |
| `CAN` | CAN 신호 정의 문서 (EZkontrol 모터컨트롤러, Daly BMS) | Markdown |
| `docs` | 사용자 문서 사이트 (ko/en) | HTML |
| `.github` | CI (펌웨어 빌드/릴리스) + 플래싱 스크립트 | YAML |

더 이상 존재하지 않는 것: `web/`(관제 웹앱), `server/`(MQTT 브로커·리버스 프록시), `device/hardware`(PCB). WiFi·MQTT·외부 RTC 모듈도 펌웨어에서 빠졌습니다.

핵심 원칙 (`CLAUDE.md`에 명시): **SD 로그 바이너리 포맷은 구조 변경 금지.** `device/firmware/main/include/protocol.h`의 24바이트 `log_t` 레이아웃, `LOG_MAGIC`(0xAE), `PROTOCOL_VERSION`(1), 체크섬 알고리즘, "BOOT 레코드가 항상 첫 레코드" 순서를 지켜야 외부 업스트림 뷰어가 파일을 읽을 수 있습니다. 필드 이름 변경(바이트는 그대로)은 괜찮지만 레이아웃/버전/매직/체크섬 변경은 사전 확인 필요.

---

# 2. 큰 그림 — 한 줄 데이터 흐름

```
[센서 태스크들] → LOG() 매크로 → logqueue (24B log_t, 큐 1개)
                                        │
                                        ▼
                                [task_sdcard]
                        /sdcard/<부팅카운터>-<시각>.log 에 순서대로 append
                        (raw 24바이트 그대로, 100Hz 풀데이터)
                                        │
                                        ▼
                (오프라인) SD카드를 빼서 외부 업스트림 사이트에 업로드 → 분석
```

큐가 **하나**뿐입니다(`logqueue`). 예전엔 MQTT 텔레메트리용 `canlogqueue`/`syslogqueue`가 따로 있었지만, 발행할 곳이 없어졌으니 모든 로그 타입(BOOT/CAN/GPS/ANALOG/DIGITAL/GYRO/SYSTEM/USER_EVENT)이 이 큐 하나로 들어가 SD에 기록됩니다.

---

# 3. 펌웨어 (`device/firmware/main/`)

## 3-1. 부팅 순서 — [main.c](device/firmware/main/main.c)
`app_main()`이 순서대로 초기화합니다:
1. `core_init()` — 온보드 RGB(GPIO38) 소등, 상태 LED(GPIO5) + `task_led` 생성, GPIO ISR 서비스 설치.
2. `nvs_init()` — 설정 영구저장소. MAC 주소 읽기, 타임존(기본 `KST-9`), CAN/GPS/analog/digital 활성화 여부, CAN 비트레이트·필터·마스크, GPS 장치종류 등 **기본값 채우기**. WiFi 자격증명(ssid/passwd/server 등) 기본값은 네트워크 제거와 함께 사라졌고, 설정 구조체(`nvs_storage_t`)도 옛 `wifi`/`device` 그룹을 걷어내고 `storage.mac`/`storage.tz` 최상위 필드로 평탄화됨.
3. `i2c0_init()` — TZ=UTC 설정, I2C0(GPIO9/10) 버스 생성(**자이로·디스플레이가 공유**), `gettimeofday(&boot,...)`로 부팅 시각 기록(GPS가 시계를 맞추기 전이라 아직 1970년 epoch 0일 수 있음).
4. `sdcard_init()` — SD 마운트 + `logqueue` 생성 + SD 기록 태스크 + BOOT 레코드 기록.
5. `peripheral_task_init()` — CAN/GPS/Analog/Digital/Gyroscope/Display 태스크 생성(설정에서 켜진 것만; Gyroscope·Display는 항상 생성).

**RTC 없음.** ESP32-S3 내장 RTC 타이머는 배터리 백업이 없어 전원이 꺼지면 시각이 사라집니다. 그래서 외부 RTC 모듈 대신 **GPS(NMEA `$GNRMC`/`$GPRMC`) 첫 픽스로 시스템 시계를 1회 설정**하고([gps.c](device/firmware/main/peripheral/gps.c)), SD의 BOOT 레코드 `boot_time`을 나중에 역보정합니다(3-4절).

**상태 시스템** ([main.h](device/firmware/main/include/main.h)): `state_t`는 32비트 비트맵. 컴포넌트는 `CORE, NVS, I2C0, SD, CAN, GPS, ANALOG, DIGITAL, GYRO` 9개, 하위 9비트=ERROR, `+12`비트=FATAL. `logbuf.run`이 현재 상태를 들고 다니고 `task_led` 점멸 속도가 최악 상태를 반영(정상 1Hz / ERROR 4Hz / FATAL 10Hz).

## 3-2. 로그의 심장 — `LOG()` 매크로 ([main.h:146](device/firmware/main/include/main.h#L146))
모든 센서 태스크가 `log_t`(24바이트)를 채우고 `LOG(타입, &log)` 한 줄로 끝냅니다. 매크로가:
- 헤더 채움 (magic 0xAE, type, timestamp=부팅후 ms)
- **체크섬** 계산 (6개 uint32 XOR → 16비트 폴딩, 8-1절)
- `logqueue`에 넣음 (`xQueueSend`, 논블로킹). (예전의 `LOG_FROM_ISR`은 digital.c가 디바운스 구조로 바뀌면서 제거 — 이제 ISR에서 직접 기록하는 곳은 없습니다)

큐는 `sdcard_init()`에서 `xQueueCreate(2560, sizeof(log_t))`로 생성되는 **`logqueue` 하나**뿐입니다.

## 3-3. 센서 태스크들 (`main/peripheral/`)
| 파일 | 역할 | 핵심 |
|------|------|------|
| [can.c](device/firmware/main/peripheral/can.c) | TWAI(CAN) 수신, GPIO15/16 | `twai_receive()` **블로킹 수신**(100ms 타임아웃) — 프레임이 없으면 잠들어 있어 유휴 CPU 점유 최소. 모든 프레임을 그대로 SD에 기록(레이트 제한 없음). `data[]`는 8바이트 고정(짧은 프레임은 0 채움, DLC>8 방어). EZ RPM·Daly SOC는 `display_can` 스냅샷에도 복사해 디스플레이가 사용 |
| [analog.c](device/firmware/main/peripheral/analog.c) | ADS1115 외장 ADC 2개(0x48/0x49), I2C1(GPIO42 SCL/47 SDA) | **두 모듈 동시 변환** 후 1.4ms 한 번만 대기(파이프라이닝). 대기도 busy-wait이 아니라 `vTaskDelay(2ms)`로 CPU 양보 후 부족분만 짧게 spin(8-7절). ain1~8 |
| [gyroscope.c](device/firmware/main/peripheral/gyroscope.c) | MPU-6050(0x68), I2C0 공유버스, 100Hz | 부팅 시 32샘플을 **1ms 간격으로 분산 수집**해 자이로 오프셋 자동 캘리브레이션(8-9절) 후 가속도/자이로 읽기 |
| [gps.c](device/firmware/main/peripheral/gps.c) | u-blox GPS, UART1(GPIO17/18) | NMEA GPRMC 파싱 → 위경도/속도/방위. **첫 유효 픽스에서 시스템 시계를 1회 설정**하고 `boot_time_fixup_epoch`를 세팅해 SD 태스크에 보정을 트리거. 파서는 콤마 누락(깨진 문장)에도 NULL 역참조 없이 안전 |
| [digital.c](device/firmware/main/peripheral/digital.c) | 디지털 입력 4채널(GPIO11~14) | **디바운스 구조**(8-4절): ISR은 마지막 엣지 시각 기록+태스크 깨우기만, 태스크가 버스트 첫 엣지 즉시 1회 + 10ms 조용해진 뒤 안정 상태 기록(변화 없으면 스킵) |
| [display.c](device/firmware/main/peripheral/display.c) | I2C LCD(PCF8574 0x27) + HD44780, I2C0 공유버스, 1Hz | **차속 계산**: CAN RPM → 기어비 4.02 + 타이어 둘레로 km/h 환산, 대형 숫자 폰트 표시. SOC%도 상단에 표시. **diff 렌더링**(8-5절): 프레임버퍼 비교로 바뀐 글자만 I2C 전송 → 같은 버스의 자이로를 방해하지 않음. PCF8574 없으면 자동 종료 |

각 태스크 공통 패턴: 샘플 → `LOG()`(SD 기록). `logbuf`에는 이제 상태 비트맵(`run`)과 digital 최종 기록 상태(`digital` — 변화 감지 비교용)만 남았습니다. 예전의 CAN/아날로그/자이로 스냅샷 필드는 읽는 곳이 없어 제거됐고, 디스플레이용 CAN 값은 별도의 `display_can` 구조체로 전달됩니다.

## 3-4. SD 기록 — [sdcard.c](device/firmware/main/peripheral/sdcard.c)
- SPI2(SCK39/MOSI40/MISO48/CS41), **SDSPI** 모드(SDMMC 아님), 클럭 **4MHz** — 점퍼선 배선에서 기본값 20MHz는 읽기 손상이 나고, 400kHz는 CAN 만부하 로깅(~50KB/s)을 못 따라가서 중간값으로 선정. (실카드 기록 → 뷰어 체크섬 검증 예정)
- 파일명은 `/sdcard/<부팅카운터 8자리>-<YYYY-MM-DD-HH-MM-SS>.log`. 부팅카운터는 NVS에 저장된 단조증가 값 — GPS가 아직 시계를 못 맞춰 `boot.tv_sec == 0`(1970년)이어도 매 부팅마다 파일명이 겹쳐 `O_TRUNC`로 이전 로그를 지우는 사고를 막아줍니다.
- 첫 레코드는 항상 **BOOT 레코드**(프로토콜 버전 + MAC + 부팅 시각).
- `task_sdcard`: 200ms마다 logqueue를 비우되, 레코드를 **64개(1536B = FAT 섹터 3개)씩 모아 한 번의 `write()`로 기록**(8-2절) — 호출당 VFS/FATFS 오버헤드를 줄이면서 파일의 바이트·순서는 개별 write와 완전히 동일. **fsync는 512건 또는 3사이클(~600ms)마다** 묶어서 → SD 마모·지연 감소. write/fsync 실패 시 FATAL 상태 전환.
- **BOOT 레코드 boot_time 역보정** (`correct_boot_record`, STEP 7): GPS가 첫 픽스로 시계를 맞추면 `gps.c`가 `boot_time_fixup_epoch`를 세팅하고, `task_sdcard`가 다음 루프에서 파일의 레코드 0(BOOT)을 되읽어 `boot_time`만 고쳐 쓰고 체크섬을 재계산합니다. 매직/타입/레이아웃은 그대로 두고 **`boot_time` 필드 값만** 바뀝니다. 파일 오프셋은 항상 EOF로 복원하므로 이어지는 append 스트림엔 영향 없음.
- **파일 = 와이어 포맷과 100% 동일**. 그래서 외부 업스트림 뷰어가 같은 24바이트 파서로 그대로 읽습니다.

---

# 4. 프로토콜 — `protocol.h`

`log_t` = 헤더 8B(`magic` 0xAE / `type` / `checksum` / `timestamp`) + 페이로드 union 16B. type별로 BOOT/CAN/GPS/ANALOG/DIGITAL/GYROSCOPE/SYSTEM/USER_EVENT ([protocol.h](device/firmware/main/include/protocol.h)).

이 구조체가 SD `.log` 파일의 바이트 그대로이자 외부 업스트림 사이트가 파싱하는 계약서입니다. 예전엔 `web/src/service/protocol.js`와 짝을 맞춰야 했지만 그 파일은 레포에서 빠졌고, **지금은 이 레포 밖의 업스트림 파서와 호환을 유지**하는 게 유일한 제약입니다. 그래서 `main.h`/`CLAUDE.md`가 강조하듯 레이아웃·매직·버전·체크섬은 바꾸면 안 됩니다.

---

# 5. 하드웨어 핀맵

보드: ESP32-S3-DevKitC-1 **v1.1** + ESP32-S3-WROOM-1 **N16R8**(16MB Flash + 8MB Octal PSRAM). 상세 배선표와 GPIO 사용 현황은 별도 문서에 정리돼 있습니다:
- [device/firmware/docs/PINMAP.md](device/firmware/docs/PINMAP.md) — 실제 배선 체크리스트(핀별 연결 대상 + 코드 근거 줄번호)
- [device/firmware/docs/GPIO_USAGE.md](device/firmware/docs/GPIO_USAGE.md) — 칩 전체 GPIO 48개의 점유/자유 상태

요약: I2C0(9/10, 자이로+LCD 공유) · I2C1(42 SCL/47 SDA, ADS1115 ×2) · TWAI(15/16) · UART1(17/18, GPS) · SPI2(39/40/41/48, SD) · GPIO11-14(디지털 입력) · GPIO5(상태 LED) · GPIO38(온보드 RGB, 소등만).

---

# 6. CI/CD (`.github/`)

| 워크플로우 | 트리거 | 역할 |
|---|---|---|
| [firmware.yml](.github/workflows/firmware.yml) | `device/firmware/**` 변경 push | ESP-IDF v6.0.1로 빌드, 산출물(`monolith.*`, bootloader, partition_table) 아티팩트 업로드 |
| [release.yml](.github/workflows/release.yml) | `v*` 태그 push | firmware.yml 호출 → 산출물 + 플래싱 스크립트를 zip으로 묶어 GitHub 릴리스 생성 |

`.github/scripts/flash.{sh,ps1,bat}`는 릴리스 zip에 포함되는 사용자용 플래싱 스크립트입니다.

---

# 7. 최적화 관점 정리

이미 들어간 설계:
- **fsync 묶기**: 512건/3사이클마다 → SD 쓰기 amplification 감소
- **ADS1115 동시변환 파이프라이닝**: 두 모듈 1.4ms 한 번으로 8채널
- **자이로 오프셋 자동 캘리브레이션**: 부팅 시 32샘플 평균으로 드리프트 제거
- **부팅카운터 기반 파일명**: 시계가 아직 안 맞아도 로그 파일이 덮어써지지 않음
- **BOOT 레코드 역보정**: 레이아웃/체크섬 규칙을 어기지 않고 `boot_time`만 사후 수정 — 전체 파일을 다시 쓰지 않아도 절대시각을 확보
- **고정 24B 레코드**: 파일 파싱이 O(1) 인덱싱
- **상태 비트맵 + LED 점멸 속도**: 배선 없이 컴포넌트별 OK/ERROR/FATAL을 바로 확인 가능

살펴볼 만한 잠재 최적화 지점:
- SD 프로빙 400kHz로 고정돼 있음 — 정상 카드면 클럭 상향 여지
- CAN 수신이 레이트 제한 없이 전부 SD로 감 — 프레임이 매우 잦은 버스에서는 큐(`logqueue`, 2560개) 포화 여부 점검 필요
