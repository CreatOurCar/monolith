#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

/***** 기어비 및 타이어 설정 *****/
#define DISPLAY_GEAR_RATIO  4.02f

// 노면 조건 전환: 0 = 건조, 1 = 우천
#define DISPLAY_WET_TRACK  0

#define DISPLAY_TIRE_DIAM_DRY_M  0.40f   // 건조: 지름 40cm → 둘레 1.2566m
#define DISPLAY_TIRE_DIAM_WET_M  0.50f   // 우천: 지름 50cm → 둘레 1.5708m

#if DISPLAY_WET_TRACK
#  define DISPLAY_TIRE_CIRC_M  (3.14159265f * DISPLAY_TIRE_DIAM_WET_M)
#else
#  define DISPLAY_TIRE_CIRC_M  (3.14159265f * DISPLAY_TIRE_DIAM_DRY_M)
#endif

#define DISPLAY_STALE_MS     2000     // 이 시간 동안 CAN 없으면 "--" 표시

/***** PCF8574 + HD44780 *****/
#define PCF8574_ADDR  0x27   // A0-A2 모두 GND 기본 주소
#define LCD_BL        0x08   // 백라이트 비트
#define LCD_EN        0x04   // Enable
#define LCD_RS        0x01   // Register Select (0=cmd, 1=data)

static const uint8_t ROW_ADDR[4] = {0x00, 0x40, 0x14, 0x54};

static i2c_master_dev_handle_t pcf8574_dev;

/***** shared CAN snapshot (defined here, declared extern in main.h) *****/
volatile display_can_t display_can = {0};

/*******************************************************************************
 * PCF8574 low-level I2C write
 ******************************************************************************/
static void pcf_write(uint8_t data) {
  i2c_master_transmit(pcf8574_dev, &data, 1, I2C_TIMEOUT_MS);
}

static void lcd_pulse_en(uint8_t data) {
  pcf_write(data | LCD_EN);
  esp_rom_delay_us(1);
  pcf_write(data & ~LCD_EN);
  esp_rom_delay_us(50);
}

static void lcd_nibble(uint8_t nibble, uint8_t flags) {
  lcd_pulse_en((uint8_t)((nibble << 4) | LCD_BL | flags));
}

/* one full byte = both nibbles' EN pulses in a single 4-byte I2C transaction
 * (PCF8574 latches its outputs after every received byte). At 100kHz each I2C
 * byte spends ~90us on the wire — longer than the HD44780's 37us execution
 * time — so no explicit delay between pulses is needed. Kept to one character
 * per transaction so the gyroscope sharing I2C0 is never blocked for long. */
static void lcd_write8(uint8_t b, uint8_t flags) {
  uint8_t hi     = (uint8_t)((b & 0xF0) | LCD_BL | flags);
  uint8_t lo     = (uint8_t)((b << 4) | LCD_BL | flags);
  uint8_t seq[4] = { hi | LCD_EN, hi, lo | LCD_EN, lo };
  i2c_master_transmit(pcf8574_dev, seq, sizeof(seq), I2C_TIMEOUT_MS);
}

static void lcd_cmd(uint8_t cmd) {
  lcd_write8(cmd, 0);
}

static void lcd_put(uint8_t b) {
  lcd_write8(b, LCD_RS);
}

static void lcd_set_cursor(uint8_t row, uint8_t col) {
  lcd_cmd(0x80 | (ROW_ADDR[row] + col));
}

/*******************************************************************************
 * LCD initialization (4-bit mode)
 ******************************************************************************/
static void lcd_init(void) {
  vTaskDelay(pdMS_TO_TICKS(50));

  lcd_nibble(0x03, 0); vTaskDelay(pdMS_TO_TICKS(5));
  lcd_nibble(0x03, 0); esp_rom_delay_us(150);
  lcd_nibble(0x03, 0); esp_rom_delay_us(150);
  lcd_nibble(0x02, 0);          // 4-bit mode

  lcd_cmd(0x28);                // 4-bit, 2-line, 5×8
  lcd_cmd(0x0C);                // display on, cursor/blink off
  lcd_cmd(0x06);                // increment, no display shift
  lcd_cmd(0x01);                // clear display
  vTaskDelay(pdMS_TO_TICKS(2));
}

/*******************************************************************************
 * custom chars for 2-row big-digit font
 * 0x08-0x0A = aliases for CGRAM slots 0-2 (avoids null byte in char arrays)
 ******************************************************************************/
#define CC_TOP  0x08   // ▀ top-half block
#define CC_BOT  0x09   // ▄ bottom-half block
#define CC_FUL  0x0A   // █ full block

static const uint8_t CC_DEF[3][8] = {
  {0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00},   // ▀
  {0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00},   // ▄
  {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00},   // █
};

static void lcd_load_custom_chars(void) {
  lcd_cmd(0x40);   // CGRAM address 0
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 8; j++)
      lcd_put(CC_DEF[i][j]);
  lcd_cmd(0x80);   // back to DDRAM
}

/*******************************************************************************
 * big-digit table: [digit][row(0=top,1=bot)][col(0..2)]
 * each digit occupies 3 chars wide × 2 rows tall
 ******************************************************************************/
static const uint8_t BIG[10][2][3] = {
  {{CC_FUL, CC_TOP, CC_FUL}, {CC_FUL, CC_BOT, CC_FUL}},   // 0
  {{' ',    CC_FUL, ' '   }, {' ',    CC_FUL, ' '   }},   // 1
  {{CC_TOP, CC_TOP, CC_FUL}, {CC_FUL, CC_BOT, CC_BOT}},   // 2
  {{CC_TOP, CC_TOP, CC_FUL}, {CC_BOT, CC_BOT, CC_FUL}},   // 3
  {{CC_FUL, CC_BOT, CC_FUL}, {' ',    ' ',    CC_FUL}},   // 4
  {{CC_FUL, CC_TOP, CC_TOP}, {CC_BOT, CC_BOT, CC_FUL}},   // 5
  {{CC_FUL, CC_TOP, CC_TOP}, {CC_FUL, CC_BOT, CC_FUL}},   // 6
  {{CC_TOP, CC_TOP, CC_FUL}, {' ',    ' ',    CC_FUL}},   // 7
  {{CC_FUL, CC_FUL, CC_FUL}, {CC_FUL, CC_BOT, CC_FUL}},   // 8
  {{CC_FUL, CC_TOP, CC_FUL}, {CC_BOT, CC_BOT, CC_FUL}},   // 9
};

/*******************************************************************************
 * render speed as 3 big digits on rows 1-2
 *   hundreds → col 4-6
 *   tens     → col 8-10
 *   units    → col 12-14
 *   gaps at col 7, 11 (always space)
 ******************************************************************************/
static void lcd_draw_speed(int spd) {
  uint8_t top[20], bot[20];
  memset(top, ' ', 20);
  memset(bot, ' ', 20);

  int h = spd / 100;
  int t = (spd / 10) % 10;
  int u = spd % 10;

  if (h) {
    top[4] = BIG[h][0][0]; top[5] = BIG[h][0][1]; top[6] = BIG[h][0][2];
    bot[4] = BIG[h][1][0]; bot[5] = BIG[h][1][1]; bot[6] = BIG[h][1][2];
  }
  if (h || t) {
    top[8]  = BIG[t][0][0]; top[9]  = BIG[t][0][1]; top[10] = BIG[t][0][2];
    bot[8]  = BIG[t][1][0]; bot[9]  = BIG[t][1][1]; bot[10] = BIG[t][1][2];
  }
  top[12] = BIG[u][0][0]; top[13] = BIG[u][0][1]; top[14] = BIG[u][0][2];
  bot[12] = BIG[u][1][0]; bot[13] = BIG[u][1][1]; bot[14] = BIG[u][1][2];

  lcd_set_cursor(1, 0);
  for (int i = 0; i < 20; i++) lcd_put(top[i]);
  lcd_set_cursor(2, 0);
  for (int i = 0; i < 20; i++) lcd_put(bot[i]);
}

static void lcd_draw_no_signal(void) {
  uint8_t row[20];
  memset(row, ' ', 20);
  const char *msg = "NO SIGNAL";
  for (int i = 0; i < 9; i++) row[6 + i] = (uint8_t)msg[i];
  lcd_set_cursor(1, 0);
  for (int i = 0; i < 20; i++) lcd_put(row[i]);
  memset(row, ' ', 20);
  lcd_set_cursor(2, 0);
  for (int i = 0; i < 20; i++) lcd_put(row[i]);
}

/*******************************************************************************
 * display refresh task — 1 Hz
 *
 * Row 0: SOC% right-aligned
 * Row 1: big digit top  ┐
 * Row 2: big digit bot  ┘  vehicle speed km/h
 * Row 3: "km/h" (written once at init)
 ******************************************************************************/
void task_display(void *pvParameters) {
  i2c_master_bus_handle_t i2c0;

  if (i2c_master_get_bus_handle(I2C_NUM_0, &i2c0) != ESP_OK) {
    ESP_LOGE("DISPLAY", "I2C0 bus not found");
    vTaskDelete(NULL);
    return;
  }

  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = PCF8574_ADDR,
    .scl_speed_hz    = 100000,
  };

  if (i2c_master_bus_add_device(i2c0, &dev_cfg, &pcf8574_dev) != ESP_OK) {
    ESP_LOGE("DISPLAY", "PCF8574 not found at 0x%02X", PCF8574_ADDR);
    vTaskDelete(NULL);
    return;
  }

  lcd_init();
  lcd_load_custom_chars();

  // row 3: static "km/h" label, centered at col 8
  lcd_set_cursor(3, 8);
  const char *kmh = "km/h";
  for (int i = 0; i < 4; i++) lcd_put((uint8_t)kmh[i]);

  TickType_t tick = xTaskGetTickCount();

  while (true) {
    vTaskDelayUntil(&tick, pdMS_TO_TICKS(1000));

    bool stale = !display_can.valid ||
                 (xTaskGetTickCount() - display_can.last_tick) > pdMS_TO_TICKS(DISPLAY_STALE_MS);

    // Row 0: SOC%, right-aligned (e.g. "          SOC:64.3%")
    {
      char soc_str[12];
      int n;
      if (stale) {
        n = 10;
        memcpy(soc_str, "SOC: --.-%", 10);
        soc_str[10] = '\0';
      } else {
        float soc = display_can.bms_soc_raw * 0.1f;
        n = snprintf(soc_str, sizeof(soc_str), "SOC:%.1f%%", soc);
      }
      lcd_set_cursor(0, 0);
      for (int i = 0; i < 20 - n; i++) lcd_put(' ');
      for (int i = 0; i < n; i++) lcd_put((uint8_t)soc_str[i]);
    }

    // Rows 1-2: vehicle speed as large digits
    if (stale) {
      lcd_draw_no_signal();
    } else {
      float rpm = (float)display_can.ez_rpm_raw * 0.1f - 2000.0f;
      if (rpm < 0.0f) rpm = -rpm;
      float spd_kmh = rpm / DISPLAY_GEAR_RATIO * DISPLAY_TIRE_CIRC_M * 60.0f / 1000.0f;
      int spd = (int)(spd_kmh + 0.5f);
      if (spd > 999) spd = 999;
      lcd_draw_speed(spd);
    }
  }
}
