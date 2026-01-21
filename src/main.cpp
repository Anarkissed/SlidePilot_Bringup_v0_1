#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>

#include "BoardPins.h"
#include "Config.h"
#include "Settings.h"
#include "Display.h"

// ------------------------------------------------------------
// LovyanGFX display driver for LilyGO T-Display S3 (I80 8-bit)
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
    {
      auto cfg = _light.config();
      cfg.pin_bl      = PIN_TFT_BL;
      cfg.invert      = false;
      cfg.freq        = 24000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }

    setPanel(&_panel);
  }
};

static LGFX_TDisplayS3 lcd;

// ------------------------------------------------------------
// Debounced inputs
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

struct RotaryEncoder {
  int pinA = -1;
  int pinB = -1;
  uint8_t prevAB = 0;
  int8_t accum = 0;
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
      detentDelta = 1;
    } else if (accum <= -(int8_t)ENC_COUNTS_PER_DETENT) {
      accum = 0;
      detentDelta = -1;
    }
  }
};

// ------------------------------------------------------------
// AS5600 (I2C)
// ------------------------------------------------------------
static constexpr uint8_t AS5600_ADDR = 0x36;

static bool i2cDevicePresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

static bool as5600ReadRawAngle(uint16_t &rawOut) {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(0x0C); // RAW_ANGLE high byte
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
// Globals
// ------------------------------------------------------------
static DebouncedInput g_btnA, g_btnB, g_encSw;
static RotaryEncoder  g_enc;

static Settings g_settings;
static DisplayUI g_display;
static UiState g_ui;

static uint16_t g_fps = 0;
static uint16_t g_frameCount = 0;
static uint32_t g_lastFpsMs = 0;

// Focus navigation ordering (HTML)
// Helper functions for new UI model
static const char* kModeNames[] = {"Single", "Bounce", "Timelapse", "Last"};
static constexpr uint8_t kModeCount = (uint8_t)(sizeof(kModeNames) / sizeof(kModeNames[0]));

static void uiSyncModeName() {
  const uint8_t idx = (uint8_t)(g_settings.lastMode % kModeCount);
  g_ui.modeName = kModeNames[idx];
}

static bool uiAllMarkersSet() {
  for (int i = 0; i < g_ui.markerCount; i++) {
    if (!g_ui.markers[i].set) return false;
  }
  return true;
}

static int uiFocusedMarkerIndex() {
  const int f = (int)g_ui.mainFocus;
  const int base = (int)MainFocus::MARKER0;
  const int idx = f - base;
  if (idx >= 0 && idx < g_ui.markerCount) return idx;
  return -1;
}

static MainFocus markerFocusFromIndex(int idx) {
  const int base = (int)MainFocus::MARKER0;
  const int v = base + idx;
  return (MainFocus)v;
}

static void mainFocusStep(int dir) {
  // Build the cyclic order: Settings -> Markers (0..N-1) -> Next -> Mode
  const int n = g_ui.markerCount;
  const int total = 1 + n + 2;

  int idx = 0;

  // Determine current index
  if (g_ui.mainFocus == MainFocus::SETTINGS) {
    idx = 0;
  } else if (g_ui.mainFocus == MainFocus::NEXT) {
    idx = 1 + n;
  } else if (g_ui.mainFocus == MainFocus::MODE) {
    idx = 1 + n + 1;
  } else {
    const int mi = uiFocusedMarkerIndex();
    idx = (mi >= 0) ? (1 + mi) : 0;
  }

  // Step with wrap
  idx += dir;
  if (idx < 0) idx = total - 1;
  if (idx >= total) idx = 0;

  // Apply new focus
  if (idx == 0) {
    g_ui.mainFocus = MainFocus::SETTINGS;
  } else if (idx >= 1 && idx <= n) {
    g_ui.mainFocus = markerFocusFromIndex(idx - 1);
  } else if (idx == 1 + n) {
    // Next can be disabled/unselectable until all markers set
    g_ui.allMarkersSet = uiAllMarkersSet();
    if (!g_ui.allMarkersSet) {
      // Skip over Next
      idx += dir;
      if (idx < 0) idx = total - 1;
      if (idx >= total) idx = 0;
      if (idx == 0) g_ui.mainFocus = MainFocus::SETTINGS;
      else if (idx >= 1 && idx <= n) g_ui.mainFocus = markerFocusFromIndex(idx - 1);
      else g_ui.mainFocus = MainFocus::MODE;
    } else {
      g_ui.mainFocus = MainFocus::NEXT;
    }
  } else {
    g_ui.mainFocus = MainFocus::MODE;
  }
}

// Marker edit presets for SET_POS screen
static int g_editMarker = -1;
static int g_editPreset = 0;

static const MarkerState kMarkerPresets[] = {
  {false,false,false,false}, // incomplete
  {true,false,false,false},  // set
  {true,true,false,false},   // ease-in
  {true,false,true,false},   // ease-out
  {true,true,true,false},    // ease-in-out
  {true,false,false,true},   // pause
  {true,true,false,true},    // ease-in + pause
  {true,false,true,true},    // pause + ease-out
  {true,true,true,true},     // ease-in + pause + ease-out
};
static constexpr int kMarkerPresetCount = (int)(sizeof(kMarkerPresets)/sizeof(kMarkerPresets[0]));

static void settingsFocusStep(int dir) {
  static const SettingsFocus order[] = {
    SettingsFocus::BACK, SettingsFocus::TESTING_MODE, SettingsFocus::INVERT_DIR
  };

  int idx = 0;
  for (int i = 0; i < (int)(sizeof(order)/sizeof(order[0])); i++) {
    if (order[i] == g_ui.settingsFocus) { idx = i; break; }
  }
  idx += dir;
  if (idx < 0) idx = (int)(sizeof(order)/sizeof(order[0])) - 1;
  if (idx >= (int)(sizeof(order)/sizeof(order[0]))) idx = 0;
  g_ui.settingsFocus = order[idx];
}

static void handleEnter() {
  switch (g_ui.screen) {
    case UiScreen::MAIN:
      switch (g_ui.mainFocus) {
        case MainFocus::SETTINGS:
          g_ui.screen = UiScreen::SETTINGS;
          g_ui.settingsFocus = SettingsFocus::TESTING_MODE;
          break;

        case MainFocus::NEXT:
          g_ui.allMarkersSet = uiAllMarkersSet();
          if (!g_ui.allMarkersSet) {
            g_ui.screen = UiScreen::POPUP;
            g_ui.popup = PopupKind::NEED_ALL_MARKERS;
            g_ui.popupYesSelected = false;
          } else {
            g_ui.screen = UiScreen::WIZARD;
          }
          break;

        case MainFocus::MODE:
          g_settings.lastMode = (uint8_t)((g_settings.lastMode + 1) % kModeCount);
          settingsSave(g_settings);
          uiSyncModeName();
          break;

        default: {
          // Any marker focus -> SET_POS (marker editing)
          const int mi = uiFocusedMarkerIndex();
          if (mi >= 0) {
            g_ui.screen = UiScreen::SET_POS;
            g_editMarker = mi;
            // Find closest preset index to current marker state
            int found = 0;
            for (int p = 0; p < kMarkerPresetCount; p++) {
              const MarkerState& s = kMarkerPresets[p];
              if (s.set==g_ui.markers[mi].set && s.easeIn==g_ui.markers[mi].easeIn && s.easeOut==g_ui.markers[mi].easeOut && s.pause==g_ui.markers[mi].pause) {
                found = p;
                break;
              }
            }
            g_editPreset = found;
          }
        } break;
      }
      break;

    case UiScreen::SETTINGS:
      if (g_ui.settingsFocus == SettingsFocus::TESTING_MODE) {
        g_settings.testingMode = !g_settings.testingMode;
        settingsSave(g_settings);
      } else if (g_ui.settingsFocus == SettingsFocus::INVERT_DIR) {
        g_settings.invertDir = !g_settings.invertDir;
        settingsSave(g_settings);
      } else if (g_ui.settingsFocus == SettingsFocus::BACK) {
        g_ui.screen = UiScreen::MAIN;
      }
      break;

    case UiScreen::SET_POS:
      // Confirm current preset for the marker and return to MAIN
      if (g_editMarker >= 0 && g_editMarker < g_ui.markerCount) {
        g_ui.markers[g_editMarker] = kMarkerPresets[g_editPreset];
      }
      g_ui.allMarkersSet = uiAllMarkersSet();
      g_ui.screen = UiScreen::MAIN;
      break;

    case UiScreen::WIZARD:
      g_ui.screen = UiScreen::RUN;
      break;

    case UiScreen::RUN:
      g_ui.screen = UiScreen::POPUP;
      g_ui.popup = PopupKind::CONFIRM_CANCEL;
      g_ui.popupYesSelected = false; // default "No"
      break;

    case UiScreen::POPUP:
      if (g_ui.popup == PopupKind::CONFIRM_CANCEL) {
        if (g_ui.popupYesSelected) {
          g_ui.screen = UiScreen::MAIN;
          g_ui.popup = PopupKind::NONE;
        } else {
          g_ui.screen = UiScreen::RUN;
          g_ui.popup = PopupKind::NONE;
        }
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("\n[SlidePilot] boot");

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

  // --- Settings ---
  settingsLoad(g_settings);
  uiSyncModeName();

  // --- LCD ---
  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);

  // --- Display UI ---
  const bool spriteOk = g_display.begin(&lcd);
  Serial.printf("[SlidePilot] DisplayUI begin: spriteOk=%s\n", spriteOk ? "true" : "false");

  // Init UI state
  g_ui.screen = UiScreen::MAIN;
  // start focus on first marker
  g_ui.markerCount = 2;
  for (int i = 0; i < UiState::kMaxMarkers; i++) {
    g_ui.markers[i] = MarkerState{};
  }

  g_ui.mainFocus = MainFocus::MARKER0;

  g_ui.settings = g_settings;

  g_ui.allMarkersSet = uiAllMarkersSet();

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

  static bool prevA=false, prevB=false, prevSW=false;
  const bool aPress  = (a && !prevA);
  const bool bPress  = (b && !prevB);
  const bool swPress = (sw && !prevSW);
  prevA=a; prevB=b; prevSW=sw;

  if (g_enc.detentDelta != 0) {
    if (g_ui.screen == UiScreen::MAIN) {
      mainFocusStep(g_enc.detentDelta);
    } else if (g_ui.screen == UiScreen::SETTINGS) {
      settingsFocusStep(g_enc.detentDelta);
    } else if (g_ui.screen == UiScreen::POPUP) {
      g_ui.popupYesSelected = !g_ui.popupYesSelected;
    } else if (g_ui.screen == UiScreen::SET_POS) {
      g_editPreset += g_enc.detentDelta;
      if (g_editPreset < 0) g_editPreset = kMarkerPresetCount - 1;
      if (g_editPreset >= kMarkerPresetCount) g_editPreset = 0;
      if (g_editMarker >= 0 && g_editMarker < g_ui.markerCount) {
        g_ui.markers[g_editMarker] = kMarkerPresets[g_editPreset];
      }
    }
  }

  if (swPress) handleEnter();

  if (aPress) {
    if (g_ui.screen == UiScreen::SETTINGS ||
        g_ui.screen == UiScreen::SET_POS ||
        g_ui.screen == UiScreen::WIZARD) {
      g_ui.screen = UiScreen::MAIN;
    } else if (g_ui.screen == UiScreen::RUN) {
      g_ui.screen = UiScreen::POPUP;
      g_ui.popup = PopupKind::CONFIRM_CANCEL;
      g_ui.popupYesSelected = false;
    } else if (g_ui.screen == UiScreen::POPUP) {
      g_ui.screen = UiScreen::RUN;
      g_ui.popup = PopupKind::NONE;
    }
  }

  (void)b; // unused right now

  As5600View av;
  av.present = i2cDevicePresent(AS5600_ADDR);
  if (av.present) {
    av.readOk = as5600ReadRawAngle(av.raw);
    if (av.readOk) av.deg = rawToDegrees(av.raw);
  }

  g_frameCount++;
  const uint32_t now = millis();
  if (now - g_lastFpsMs >= 1000) {
    g_fps = g_frameCount;
    g_frameCount = 0;
    g_lastFpsMs = now;
  }

  g_ui.settings = g_settings;
  uiSyncModeName();
  g_ui.allMarkersSet = uiAllMarkersSet();
  g_ui.as5600 = av;
  g_ui.fps = g_fps;
  g_ui.btnA = a; g_ui.btnB = b; g_ui.encSw = sw;

  static uint32_t lastUiMs = 0;
  if (now - lastUiMs >= UI_UPDATE_MS) {
    lastUiMs = now;
    g_display.render(g_ui);
  }

  delay(INPUT_POLL_DELAY_MS);
}