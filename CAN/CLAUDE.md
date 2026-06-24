이 저장소는 luftaquila/monolith(Monolith v2)를 포크한 것이다. ESP32-S3 단일 칩으로 자작 포뮬러 차량의 텔레메트리를 수집하는 로거다.

[구조]
- 펌웨어: device/firmware/, ESP-IDF v5.5.1, C. ESP32가 CAN + 센서를 수집해 SD카드(.log)에 기록하고 WiFi로 MQTT 발행한다.
- 웹: web/, Vue. 로컬 Mosquitto 브로커에 붙어 실시간(Telemetry) 표시 + SD 로그를 뷰어(Viewer)로 분석.

[현재 상태]
- 웹앱은 로컬 브로커 연결·브랜딩·End-to-End(가짜 발행기) 검증 완료. MQTT 채널명 = testcar.
- ESP32 보드가 아직 도착 전이라 플래시/하드웨어 테스트 불가. 지금은 코드 리뷰·설정·작성(스캐폴딩)까지만 하고, 하드웨어가 있어야 검증되는 부분은 따로 표시할 것.

[사용 센서 — 전부 사용]
- CAN: TWAI, 트랜시버 Adafruit CAN Pal(TJA1051T/3)
- GPS: BN-880, UART1, NMEA GPRMC
- IMU: MPU6500, I2C 0x68 (MPU6050 호환 레지스터맵)
- 아날로그: 외부 I2C ADC(내장 ADC 아님), SCL=GPIO48 / SDA=GPIO47 — 선형 포텐셔미터 등
- 디지털(휠스피드): GPIO11~14, ANYEDGE 인터럽트, 내부풀업 OFF (옵토커플러 경유)

[새로 코딩해야 하는 것 2가지]
1. 계기판 로컬 디스플레이 — Monolith엔 로컬 화면 출력이 없음. 디스플레이 드라이버 + 센서값 렌더를 ESP-IDF로 추가해야 함. (모듈 모델 추후 제공)
2. CAN 디코더 — 펌웨어가 아니라 web/src/service/ui.js에 있음. CAN ID·바이트 → 신호명/단위/배율 매핑. (차량 DBC 추후 제공)

[설정 작업 — 새 코드 아님]
esp32s3 타깃, 핀 매핑(TWAI/SD), 사용 센서 enable, CAN 비트레이트/필터.
주의: WiFi/서버 정보는 컴파일에 넣지 말 것 — 런타임 SoftAP(192.168.4.1) 방식이다.

[원칙]
로그 포맷을 바꾸면 펌웨어 main.h 와 web/src/service/protocol.js 를 같은 변경으로 함께 수정해야 한다.