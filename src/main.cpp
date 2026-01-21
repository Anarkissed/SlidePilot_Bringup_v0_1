#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>

// Pins: single source of truth.
#include "BoardPins.h"

// Config constants: single source of truth.
#include "Config.h"

// Persisted settings
#include "Settings.h"

// ------------------------------------------------------------
// Settings persistence implementation (Milestone 3)
// NOTE: Implemented here to guarantee the linker sees the definitions.
// Later, we can move this into src/Settings.cpp once the file exists.
// ------------------------------------------------------------
static Preferences g_prefs;

void settingsSetDefaults(Settings& s) {
  s.testingMode = false;
  s.invertDir   = false;
  s.lastMode    = 0;
  s.bootCount   = 0;
}

bool settingsLoad(Settings& s) {
  settingsSetDefaults(s);

  if (!g_prefs.begin(PREFS_NAMESPACE, false)) {
    return false;
  }

  const uint16_t ver = g_prefs.getUShort(KEY_VER, 0);
  if (ver != SETTINGS_VERSION) {
    // Version mismatch or first boot: write defaults.
    settingsSetDefaults(s);
    g_prefs.putUShort(KEY_VER, SETTINGS_VERSION);
    g_prefs.putBool(KEY_TEST, s.testingMode);
    g_prefs.putBool(KEY_INVERT, s.invertDir);
    g_prefs.putUChar(KEY_LASTMODE, s.lastMode);
    g_prefs.putUInt(KEY_BOOTCOUNT, s.bootCount);
    g_prefs.end();
    return true;
  }

  s.testingMode = g_prefs.getBool(KEY_TEST, s.testingMode);
  s.invertDir   = g_prefs.getBool(KEY_INVERT, s.invertDir);
  s.lastMode    = g_prefs.getUChar(KEY_LASTMODE, s.lastMode);
  s.bootCount   = g_prefs.getUInt(KEY_BOOTCOUNT, s.bootCount);

  g_prefs.end();
  return true;
}

bool settingsSave(const Settings& s) {
  if (!g_prefs.begin(PREFS_NAMESPACE, false)) {
    return false;
  }

  g_prefs.putUShort(KEY_VER, SETTINGS_VERSION);
  g_prefs.putBool(KEY_TEST, s.testingMode);
  g_prefs.putBool(KEY_INVERT, s.invertDir);
  g_prefs.putUChar(KEY_LASTMODE, s.lastMode);
  g_prefs.putUInt(KEY_BOOTCOUNT, s.bootCount);

  g_prefs.end();
  return true;
}

// ------------------------------------------------------------
// LovyanGFX display driver for LilyGO T-Display S3
// ------------------------------------------------------------
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
    // Fix rolling "dark band" by raising PWM frequency well above visible range.
    {
      auto cfg = _light.config();
      cfg.pin_bl      = PIN_TFT_BL;
      cfg.invert      = false;
      cfg.freq        = 50000;   // was 12000; 50kHz reduces visible banding
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
// AS5600 helpers (still displayed)
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
// UI
// ------------------------------------------------------------
static void flashTestPattern() {
  lcd.fillScreen(lcd.color888(255, 0, 0)); delay(120);
  lcd.fillScreen(lcd.color888(0, 255, 0)); delay(120);
  lcd.fillScreen(lcd.color888(0, 0, 255)); delay(120);
  lcd.fillScreen(lcd.color888(0, 0, 0));   delay(120);
}

static void drawStaticLayout() {
  lcd.fillScreen(lcd.color888(0, 0, 0));

  // Removed: title line + "Milestone 3..." line (per request)
  // Keep only one compact instruction line to maximize vertical space.
  lcd.setTextSize(1);
  lcd.setTextColor(lcd.color888(255, 255, 255));
  lcd.setCursor(6, 4);
  lcd.print("SW: test  |  B: invert  |  ENC: mode  |  A: save");

  // Divider line
  lcd.drawFastHLine(0, 18, lcd.width(), lcd.color888(60, 60, 60));
}

static void drawLiveStatus(
  const Settings& s,
  bool btnA, bool btnB, bool encSw,
  int32_t encPos, int8_t encDelta,
  bool as5600Present, bool as5600ReadOk, uint16_t raw, float deg,
  uint16_t fps,
  bool showSaved
) {
  // Clear live status area (below divider)
  lcd.fillRect(0, 20, lcd.width(), lcd.height() - 20, lcd.color888(0, 0, 0));

  lcd.setTextSize(2);
  lcd.setCursor(8, 24);
  lcd.setTextColor(lcd.color888(255, 255, 0));
  lcd.printf("A:%s  B:%s  SW:%s", btnA ? "ON" : "--", btnB ? "ON" : "--", encSw ? "ON" : "--");

  lcd.setCursor(8, 48);
  lcd.setTextColor(lcd.color888(0, 255, 255));
  lcd.printf("ENC:%ld  d:%d", (long)encPos, (int)encDelta);

  lcd.setTextSize(1);
  lcd.setTextColor(lcd.color888(180, 180, 180));

  lcd.setCursor(8, 74);
  lcd.printf("testingMode: %s", s.testingMode ? "true" : "false");

  lcd.setCursor(8, 88);
  lcd.printf("invertDir:   %s", s.invertDir ? "true" : "false");

  lcd.setCursor(8, 102);
  lcd.printf("lastMode:    %u", (unsigned)s.lastMode);

  lcd.setCursor(8, 116);
  lcd.printf("bootCount:   %lu", (unsigned long)s.bootCount);

  lcd.setCursor(8, 134);
  if (!as5600Present) {
    lcd.print("AS5600: NOT FOUND @0x36");
  } else if (!as5600ReadOk) {
    lcd.print("AS5600: present, read FAIL");
  } else {
    lcd.printf("AS5600: raw=0x%03X  deg=%0.1f", raw, deg);
  }

  lcd.setCursor(8, 150);
  lcd.printf("FPS: %u", (unsigned)fps);

  if (showSaved) {
    lcd.setTextSize(2);
    lcd.setTextColor(lcd.color888(0, 255, 0));
    lcd.setCursor(8, 170);
    lcd.print("SAVED");
  }
}

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
static DebouncedInput g_btnA;
static DebouncedInput g_btnB;
static DebouncedInput g_encSw;
static RotaryEncoder  g_enc;

static Settings g_settings;

static uint16_t g_fps = 0;
static uint16_t g_frameCount = 0;
static uint32_t g_lastFpsMs = 0;

static uint32_t g_savedToastUntil = 0;

static void markSavedToast() {
  g_savedToastUntil = millis() + SAVED_TOAST_MS;
}

static void clampLastMode(Settings& s) {
  if (s.lastMode < LASTMODE_MIN) s.lastMode = LASTMODE_MIN;
  if (s.lastMode > LASTMODE_MAX) s.lastMode = LASTMODE_MAX;
}

void setup() {
  Serial.begin(115200);
  delay(50);

  // --- Motor safety: keep driver disabled + prevent floating STEP/DIR ---
  pinMode(PIN_TMC_STEP, OUTPUT); digitalWrite(PIN_TMC_STEP, LOW);
  pinMode(PIN_TMC_DIR,  OUTPUT); digitalWrite(PIN_TMC_DIR,  LOW);
  pinMode(PIN_TMC_EN,   OUTPUT); digitalWrite(PIN_TMC_EN,   HIGH); // Active-LOW => HIGH disables
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

  // --- Settings load + boot counter ---
  settingsLoad(g_settings);
  g_settings.bootCount++;
  settingsSave(g_settings);
  markSavedToast();

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

  const bool a = g_btnA.pressedActiveLow();
  const bool b = g_btnB.pressedActiveLow();
  const bool sw = g_encSw.pressedActiveLow();

  // Edge detection
  static bool prevA = false, prevB = false, prevSW = false;
  const bool aPress = (a && !prevA);
  const bool bPress = (b && !prevB);
  const bool swPress = (sw && !prevSW);
  prevA = a; prevB = b; prevSW = sw;

  bool changed = false;

  // Encoder changes lastMode
  if (g_enc.detentDelta != 0) {
    int newMode = (int)g_settings.lastMode + (int)g_enc.detentDelta;
    if (newMode < (int)LASTMODE_MIN) newMode = LASTMODE_MIN;
    if (newMode > (int)LASTMODE_MAX) newMode = LASTMODE_MAX;
    if ((uint8_t)newMode != g_settings.lastMode) {
      g_settings.lastMode = (uint8_t)newMode;
      changed = true;
    }
  }

  // SW toggles testingMode
  if (swPress) {
    g_settings.testingMode = !g_settings.testingMode;
    changed = true;
  }

  // B toggles invertDir
  if (bPress) {
    g_settings.invertDir = !g_settings.invertDir;
    changed = true;
  }

  // A forces save
  if (aPress) {
    settingsSave(g_settings);
    markSavedToast();
  }

  if (changed) {
    clampLastMode(g_settings);
    settingsSave(g_settings);
    markSavedToast();
  }

  // AS5600
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
    const bool showSaved = (now < g_savedToastUntil);
    drawLiveStatus(
      g_settings,
      a, b, sw,
      g_enc.detentPos, g_enc.detentDelta,
      as5600_present, as5600_read_ok, raw, deg,
      g_fps,
      showSaved
    );
  }

  delay(INPUT_POLL_DELAY_MS);
}