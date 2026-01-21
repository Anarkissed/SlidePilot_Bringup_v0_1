#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>

// Pins: single source of truth.
#include "BoardPins.h"

// Config constants: single source of truth.
#include "Config.h"

// ------------------------------------------------------------
// LovyanGFX display driver for LilyGO T-Display S3
// ------------------------------------------------------------
// ST7789 panel on 8-bit 8080 (I80) parallel bus.
class LGFX_TDisplayS3 : public lgfx::LGFX_Device {
 public:
  lgfx::Panel_ST7789   _panel;
  lgfx::Bus_Parallel8  _bus;
  lgfx::Light_PWM      _light;

  LGFX_TDisplayS3() {
    // ---- 8-bit Parallel Bus (I80) ----
    {
      auto cfg = _bus.config();
      cfg.freq_write = 20000000;
      cfg.freq_read  = 16000000;

      cfg.pin_wr = PIN_LCD_WR;
      cfg.pin_rd = PIN_LCD_RD;
      cfg.pin_rs = PIN_LCD_DC;

      cfg.pin_d0 = PIN_LCD_D0;
      cfg.pin_d1 = PIN_LCD_D1;
      cfg.pin_d2 = PIN_LCD_D2;
      cfg.pin_d3 = PIN_LCD_D3;
      cfg.pin_d4 = PIN_LCD_D4;
      cfg.pin_d5 = PIN_LCD_D5;
      cfg.pin_d6 = PIN_LCD_D6;
      cfg.pin_d7 = PIN_LCD_D7;

      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    // ---- Panel (ST7789) ----
    {
      auto cfg = _panel.config();
      cfg.pin_cs   = PIN_LCD_CS;
      cfg.pin_rst  = PIN_LCD_RST;
      cfg.pin_busy = -1;

      cfg.panel_width  = 170;
      cfg.panel_height = 320;

      cfg.offset_x = 35;
      cfg.offset_y = 0;

      cfg.offset_rotation = 0;
      cfg.readable   = false;
      cfg.invert     = true;
      cfg.rgb_order  = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel.config(cfg);
    }

    // ---- Backlight PWM ----
    {
      auto cfg = _light.config();
      cfg.pin_bl      = PIN_TFT_BL;
      cfg.invert      = false;
      cfg.freq        = 12000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }

    setPanel(&_panel);
  }
};

static LGFX_TDisplayS3 lcd;

// ------------------------------------------------------------
// Debounced digital input helper
// ------------------------------------------------------------
struct DebouncedInput {
  int pin = -1;
  bool stable = true;
  bool lastRaw = true;
  uint32_t lastChangeMs = 0;
  uint32_t debounceMs = 25;

  void begin(int gpio, bool pullup, uint32_t debounce) {
    pin = gpio;
    debounceMs = debounce;
    pinMode(pin, pullup ? INPUT_PULLUP : INPUT);

    const bool raw = digitalRead(pin);
    stable = raw;
    lastRaw = raw;
    lastChangeMs = millis();
  }

  void update() {
    const bool raw = digitalRead(pin);
    if (raw != lastRaw) {
      lastRaw = raw;
      lastChangeMs = millis();
    }
    const uint32_t now = millis();
    if ((now - lastChangeMs) >= debounceMs) {
      stable = lastRaw;
    }
  }

  bool pressedActiveLow() const { return stable == LOW; }
};

// ------------------------------------------------------------
// Rotary encoder (polling) using Gray-code transition table
// ------------------------------------------------------------
struct RotaryEncoder {
  int pinA = -1;
  int pinB = -1;
  uint8_t prevAB = 0;
  int8_t accum = 0;
  int32_t detentPos = 0;
  int8_t detentDelta = 0;

  void begin(int gpioA, int gpioB) {
    pinA = gpioA;
    pinB = gpioB;
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);

    const uint8_t a = (uint8_t)digitalRead(pinA);
    const uint8_t b = (uint8_t)digitalRead(pinB);
    prevAB = (a << 1) | b;

    accum = 0;
    detentPos = 0;
    detentDelta = 0;
  }

  void update() {
    detentDelta = 0;

    const uint8_t a = (uint8_t)digitalRead(pinA);
    const uint8_t b = (uint8_t)digitalRead(pinB);
    const uint8_t currAB = (a << 1) | b;

    if (currAB == prevAB) return;

    static const int8_t kTable[16] = {
      0, -1,  1,  0,
      1,  0,  0, -1,
     -1,  0,  0,  1,
      0,  1, -1,  0
    };

    const uint8_t idx = (prevAB << 2) | currAB;
    const int8_t delta = kTable[idx];
    prevAB = currAB;

    if (delta == 0) return;

    accum += delta;

    if (accum >= (int8_t)ENC_COUNTS_PER_DETENT) {
      accum = 0;
      detentPos++;
      detentDelta = 1;
    } else if (accum <= -(int8_t)ENC_COUNTS_PER_DETENT) {
      accum = 0;
      detentPos--;
      detentDelta = -1;
    }
  }
};

// ------------------------------------------------------------
// AS5600 helpers
// ------------------------------------------------------------
static constexpr uint8_t AS5600_ADDR = 0x36;

static bool i2cDevicePresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

static bool as5600ReadRawAngle(uint16_t &rawOut) {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(0x0C);
  if (Wire.endTransmission(false) != 0) return false;

  const uint8_t n = Wire.requestFrom((int)AS5600_ADDR, 2);
  if (n != 2) return false;

  const uint8_t msb = Wire.read();
  const uint8_t lsb = Wire.read();
  rawOut = (((uint16_t)msb << 8) | lsb) & 0x0FFF;
  return true;
}

static float rawToDegrees(uint16_t raw) {
  return (raw * 360.0f) / 4096.0f;
}

// ------------------------------------------------------------
// UI drawing
// ------------------------------------------------------------
static void flashTestPattern() {
  lcd.fillScreen(lcd.color888(255, 0, 0)); delay(150);
  lcd.fillScreen(lcd.color888(0, 255, 0)); delay(150);
  lcd.fillScreen(lcd.color888(0, 0, 255)); delay(150);
  lcd.fillScreen(lcd.color888(0, 0, 0));   delay(150);
}

static void drawStaticLayout() {
  lcd.fillScreen(lcd.color888(0, 0, 0));

  lcd.setTextSize(2);
  lcd.setTextColor(lcd.color888(255, 255, 255));
  lcd.setCursor(8, 8);
  lcd.print("SlidePilot Bring-up");

  lcd.setTextSize(1);
  lcd.setCursor(8, 36);
  lcd.print("Buttons: A=GPIO0  B=GPIO14 (LOW=pressed)");

  lcd.setCursor(8, 50);
  lcd.print("Encoder: CLK=GPIO17 DT=GPIO18 SW=GPIO1");

  lcd.setCursor(8, 64);
  lcd.print("I2C: SDA=GPIO21  SCL=GPIO16");

  lcd.drawFastHLine(0, 78, lcd.width(), lcd.color888(60, 60, 60));
}

static void drawLiveStatus(
  bool btnA, bool btnB,
  bool encSw, int32_t encPos, int8_t encDelta,
  bool as5600Present, bool as5600ReadOk, uint16_t raw, float deg,
  uint16_t fps
) {
  lcd.fillRect(0, 82, lcd.width(), lcd.height() - 82, lcd.color888(0, 0, 0));

  lcd.setTextSize(2);
  lcd.setCursor(8, 88);
  lcd.setTextColor(lcd.color888(255, 255, 0));
  lcd.printf("A:%s  B:%s", btnA ? "ON" : "--", btnB ? "ON" : "--");

  lcd.setCursor(8, 112);
  lcd.setTextColor(lcd.color888(0, 255, 255));
  lcd.printf("ENC:%ld  d:%d  SW:%s", (long)encPos, (int)encDelta, encSw ? "ON" : "--");

  lcd.setTextSize(1);
  lcd.setTextColor(lcd.color888(180, 180, 180));

  lcd.setCursor(8, 140);
  if (!as5600Present) {
    lcd.print("AS5600: NOT FOUND @0x36");
  } else if (!as5600ReadOk) {
    lcd.print("AS5600: present, read FAIL");
  } else {
    lcd.printf("AS5600: raw=0x%03X  deg=%0.2f", raw, deg);
  }

  lcd.setCursor(8, 156);
  lcd.printf("FPS: %u", (unsigned)fps);

  lcd.setCursor(8, 174);
  lcd.print("(Milestone 2: inputs debounced)");
}

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
static DebouncedInput g_btnA;
static DebouncedInput g_btnB;
static DebouncedInput g_encSw;
static RotaryEncoder  g_enc;

static uint16_t g_fps = 0;
static uint16_t g_frameCount = 0;
static uint32_t g_lastFpsMs = 0;

void setup() {
  Serial.begin(115200);
  delay(50);

  // --- Motor safety: keep driver disabled + prevent floating STEP/DIR ---
  pinMode(PIN_TMC_STEP, OUTPUT); digitalWrite(PIN_TMC_STEP, LOW);
  pinMode(PIN_TMC_DIR,  OUTPUT); digitalWrite(PIN_TMC_DIR,  LOW);
  pinMode(PIN_TMC_EN,   OUTPUT); digitalWrite(PIN_TMC_EN,   HIGH); // Active-LOW enable => HIGH disables
  pinMode(PIN_TMC_UART_TX, INPUT);
  pinMode(PIN_TMC_UART_RX, INPUT);

  // --- TFT power/backlight ---
  pinMode(PIN_TFT_POWER, OUTPUT); digitalWrite(PIN_TFT_POWER, HIGH);
  pinMode(PIN_TFT_BL,    OUTPUT); digitalWrite(PIN_TFT_BL,    HIGH);

  // --- Inputs ---
  g_btnA.begin(PIN_BTN_A, true, BTN_DEBOUNCE_MS);
  g_btnB.begin(PIN_BTN_B, true, BTN_DEBOUNCE_MS);
  g_encSw.begin(PIN_ENC_SW, true, ENC_SW_DEBOUNCE_MS);
  g_enc.begin(PIN_ENC_A, PIN_ENC_B);

  // --- I2C ---
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  // --- LCD ---
  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);

  flashTestPattern();
  drawStaticLayout();

  g_lastFpsMs = millis();
}

void loop() {
  g_btnA.update();
  g_btnB.update();
  g_encSw.update();
  g_enc.update();

  const bool btnA_pressed  = g_btnA.pressedActiveLow();
  const bool btnB_pressed  = g_btnB.pressedActiveLow();
  const bool encSw_pressed = g_encSw.pressedActiveLow();

  const bool as5600_present = i2cDevicePresent(AS5600_ADDR);
  uint16_t raw = 0;
  float deg = 0.0f;
  bool as5600_read_ok = false;

  if (as5600_present) {
    as5600_read_ok = as5600ReadRawAngle(raw);
    if (as5600_read_ok) deg = rawToDegrees(raw);
  }

  // FPS
  g_frameCount++;
  const uint32_t now = millis();
  if (now - g_lastFpsMs >= 1000) {
    g_fps = g_frameCount;
    g_frameCount = 0;
    g_lastFpsMs = now;
  }

  // UI refresh
  static uint32_t lastUiMs = 0;
  if (now - lastUiMs >= UI_UPDATE_MS) {
    lastUiMs = now;
    drawLiveStatus(
      btnA_pressed, btnB_pressed,
      encSw_pressed, g_enc.detentPos, g_enc.detentDelta,
      as5600_present, as5600_read_ok, raw, deg,
      g_fps
    );
  }

  delay(INPUT_POLL_DELAY_MS);
}