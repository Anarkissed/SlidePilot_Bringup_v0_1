// src/main.cpp
// SlidePilot Bring-up → Real UI scaffold
//
// Goals:
//  - Keep TFT powered + backlight on (always)
//  - Render the UI with tear-free refresh (sprite/double-buffer)
//  - Provide basic rotary encoder navigation + click + back button
//
// NOTE: This is still "bring-up" firmware. It focuses on UI correctness first.

#include <Arduino.h>
#include <LovyanGFX.hpp>

#include "BoardPins.h"
#include "Display.h"

// Optional Roboto font loading (LittleFS). If the font isn't flashed, we fall
// back to built-in fonts cleanly.
#include <LittleFS.h>

// -----------------------------
// LovyanGFX device (Parallel8 -> ST7789)
// -----------------------------
class LGFX_TDisplayS3 : public lgfx::LGFX_Device {
  lgfx::Bus_Parallel8 _bus;
  lgfx::Panel_ST7789  _panel;
  lgfx::Light_PWM     _light;

public:
  LGFX_TDisplayS3() {
    // ---- 8-bit Parallel Bus (I80) ----
    {
      auto cfg = _bus.config();

      cfg.port       = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read  = 16000000;

      cfg.pin_wr = PIN_LCD_WR;
      cfg.pin_rd = PIN_LCD_RD;
      cfg.pin_rs = PIN_LCD_DC;  // RS/DC

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

    // ---- Panel ----
    {
      auto pcfg = _panel.config();

      pcfg.pin_cs   = PIN_LCD_CS;
      pcfg.pin_rst  = PIN_LCD_RST;
      pcfg.pin_busy = -1;

      pcfg.panel_width  = 170;
      pcfg.panel_height = 320;

      // Known-good offsets for T-Display S3 ST7789
      pcfg.offset_x = 35;
      pcfg.offset_y = 0;

      pcfg.offset_rotation = 0;
      pcfg.readable        = false;
      pcfg.invert          = true;
      pcfg.rgb_order       = false;
      pcfg.dlen_16bit      = false;
      pcfg.bus_shared      = false;

      _panel.config(pcfg);
    }

    // ---- Backlight ----
    {
      auto lcfg = _light.config();
      lcfg.pin_bl      = PIN_TFT_BL;
      lcfg.invert      = false;
      lcfg.freq        = 12000;
      lcfg.pwm_channel = 7;
      _light.config(lcfg);
      _panel.setLight(&_light);
    }

    setPanel(&_panel);
  }
};

static LGFX_TDisplayS3 lcd;
static DisplayUI       ui;
static UiState         state;

// -----------------------------------------------------------------------------
// Simple input (poll-based)
// -----------------------------------------------------------------------------

static bool readButtonActiveLow(int pin) {
  return digitalRead(pin) == LOW;
}

static int8_t encoderStep() {
  // KY-040 polling decode (good enough for UI navigation)
  static uint8_t last = 0;
  uint8_t a = digitalRead(PIN_ENC_A) ? 1 : 0;
  uint8_t b = digitalRead(PIN_ENC_B) ? 1 : 0;
  uint8_t now = (a << 1) | b;

  int8_t step = 0;
  // Gray-code transitions
  if ((last == 0b00 && now == 0b01) || (last == 0b01 && now == 0b11) || (last == 0b11 && now == 0b10) || (last == 0b10 && now == 0b00)) {
    step = +1;
  } else if ((last == 0b00 && now == 0b10) || (last == 0b10 && now == 0b11) || (last == 0b11 && now == 0b01) || (last == 0b01 && now == 0b00)) {
    step = -1;
  }

  last = now;
  return step;
}

static bool encoderClicked() {
  static bool last = false;
  static uint32_t lastMs = 0;

  bool cur = readButtonActiveLow(PIN_ENC_SW);
  uint32_t ms = millis();

  bool rising = (cur && !last);
  last = cur;

  if (rising && (ms - lastMs) > 180) {
    lastMs = ms;
    return true;
  }
  return false;
}

static bool backPressed() {
  static bool last = false;
  static uint32_t lastMs = 0;

  bool cur = readButtonActiveLow(PIN_BTN_A);
  uint32_t ms = millis();
  bool rising = (cur && !last);
  last = cur;
  if (rising && (ms - lastMs) > 180) {
    lastMs = ms;
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// Navigation helpers
// -----------------------------------------------------------------------------

static SlideMode nextMode(SlideMode m) {
  switch (m) {
    case SlideMode::Single:    return SlideMode::Bounce;
    case SlideMode::Bounce:    return SlideMode::Timelapse;
    case SlideMode::Timelapse: return SlideMode::Single;
  }
  return SlideMode::Single;
}

static void mainFocusMove(int dir) {
  int f = static_cast<int>(state.mainFocus);
  const int first = static_cast<int>(MainFocus::Settings);
  const int last  = static_cast<int>(MainFocus::Next);
  f += dir;
  if (f < first) f = last;
  if (f > last)  f = first;
  state.mainFocus = static_cast<MainFocus>(f);

  // When focus lands on a marker, keep selectedMarker matched
  if (state.mainFocus >= MainFocus::Marker1 && state.mainFocus <= MainFocus::Marker6) {
    int idx = static_cast<int>(state.mainFocus) - static_cast<int>(MainFocus::Marker1);
    idx = constrain(idx, 0, (int)UiState::kMaxMarkers - 1);
    state.selectedMarker = idx;
  }
}

static void setPosFocusMove(int dir) {
  int f = static_cast<int>(state.setPosFocus);
  const int first = static_cast<int>(SetPosFocus::Center);
  const int last  = static_cast<int>(SetPosFocus::Back);
  f += dir;
  if (f < first) f = last;
  if (f > last)  f = first;
  state.setPosFocus = static_cast<SetPosFocus>(f);
}

static void markerMenuFocusMove(int dir) {
  // Movement → Pause → (PauseSeconds if enabled) → Back → wrap
  if (state.editingPauseSeconds) {
    // while editing, rotation edits value, not focus
    return;
  }

  auto next = [&](MarkerMenuFocus cur, int dirStep) {
    // Build active list
    MarkerMenuFocus list[4];
    int n = 0;
    list[n++] = MarkerMenuFocus::Movement;
    list[n++] = MarkerMenuFocus::Pause;
    if (state.markers[state.selectedMarker].pauseEnabled) {
      list[n++] = MarkerMenuFocus::PauseSeconds;
    }
    list[n++] = MarkerMenuFocus::Back;

    int idx = 0;
    for (int i = 0; i < n; ++i) {
      if (list[i] == cur) { idx = i; break; }
    }
    idx += dirStep;
    if (idx < 0) idx = n - 1;
    if (idx >= n) idx = 0;
    return list[idx];
  };

  state.markerMenuFocus = next(state.markerMenuFocus, dir);
}

static void enterSetPosition(uint8_t markerIdx) {
  state.selectedMarker = markerIdx;
  state.setPosFocus = SetPosFocus::Center;
  state.screen = UiScreen::SetPosition;
}

static void enterMarkerMenu() {
  state.markerMenuFocus = MarkerMenuFocus::Movement;
  state.editingPauseSeconds = false;
  state.screen = UiScreen::MarkerMenu;
}

static void leaveToMain() {
  state.screen = UiScreen::Main;
  state.mainFocus = MainFocus::Settings;
}

// -----------------------------------------------------------------------------
// Power + init
// -----------------------------------------------------------------------------

static void powerOnDisplay() {
  pinMode(PIN_TFT_POWER, OUTPUT);
  digitalWrite(PIN_TFT_POWER, HIGH);

  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
}

static void initInputs() {
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
}

static void tryLoadRoboto() {
  // Place Roboto in data/Roboto-Regular.ttf and upload LittleFS:
  // pio run -t uploadfs
  if (!LittleFS.begin(true)) {
    return;
  }
  // LovyanGFX loadFont works if FS is mounted.
  // If it fails, it just returns false and we keep default font.
  lcd.loadFont("/Roboto-Regular.ttf");
}

void setup() {
  delay(50);

  powerOnDisplay();
  initInputs();
  delay(50);

  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);

  tryLoadRoboto();

  ui.begin(&lcd);

  // Initial state
  state = UiState{};
  state.screen = UiScreen::Main;
  state.mode = SlideMode::Single;
  state.mainFocus = MainFocus::Settings;
  state.markerCount = 2;

  state.markers[0].set = true;
  state.markers[0].posPercent = 20;
  state.markers[1].set = false;
  state.markers[1].posPercent = 50;
}

void loop() {
  // HARD FORCE backlight on every loop (prevents random dim/blank)
  lcd.setBrightness(255);

  const int8_t step = encoderStep();
  const bool click = encoderClicked();
  const bool back  = backPressed();

  // -------------------
  // INPUT → STATE
  // -------------------

  if (back) {
    // Global back behavior
    if (state.screen == UiScreen::Settings) {
      leaveToMain();
    } else if (state.screen == UiScreen::SetPosition) {
      // Back saves & exits
      leaveToMain();
    } else if (state.screen == UiScreen::MarkerMenu) {
      state.screen = UiScreen::SetPosition;
      state.setPosFocus = SetPosFocus::Options;
    }
  }

  switch (state.screen) {
    case UiScreen::Main: {
      if (step) mainFocusMove(step);

      if (click) {
        if (state.mainFocus == MainFocus::Settings) {
          state.screen = UiScreen::Settings;
          state.settingsSelection = 0;
        } else if (state.mainFocus >= MainFocus::Marker1 && state.mainFocus <= MainFocus::Marker6) {
          uint8_t idx = state.selectedMarker;
          enterSetPosition(idx);
        } else if (state.mainFocus == MainFocus::Mode) {
          // Mode cycles instantly: Single → Bounce → Timelapse → Single
          state.mode = nextMode(state.mode);
        } else if (state.mainFocus == MainFocus::Next) {
          // Bring-up placeholder
          state.screen = UiScreen::Running;
        }
      }
    } break;

    case UiScreen::Settings: {
      if (step) {
        int sel = (int)state.settingsSelection + step;
        if (sel < 0) sel = 1;
        if (sel > 1) sel = 0;
        state.settingsSelection = (uint8_t)sel;
      }

      if (click) {
        if (state.settingsSelection == 0) {
          // Add Marker
          if (state.markerCount < UiState::kMaxMarkers) {
            state.markerCount++;
            // new marker defaults to unset at far right
            state.markers[state.markerCount - 1].set = false;
            state.markers[state.markerCount - 1].posPercent = 80;
          }
        } else {
          leaveToMain();
        }
      }
    } break;

    case UiScreen::SetPosition: {
      if (state.setPosFocus == SetPosFocus::Center) {
        // rotation changes marker position
        if (step) {
          auto& mk = state.markers[state.selectedMarker];
          int v = (int)mk.posPercent + step;
          v = constrain(v, 0, 100);
          mk.posPercent = (uint8_t)v;
        }
        if (click) {
          // click sets marker and jumps focus to Back pill
          state.markers[state.selectedMarker].set = true;
          state.setPosFocus = SetPosFocus::Back;
        }
      } else {
        if (step) setPosFocusMove(step);
        if (click) {
          if (state.setPosFocus == SetPosFocus::Options) {
            enterMarkerMenu();
          } else if (state.setPosFocus == SetPosFocus::Back) {
            // Back saves
            state.markers[state.selectedMarker].set = true;
            leaveToMain();
          }
        }
      }
    } break;

    case UiScreen::MarkerMenu: {
      auto& mk = state.markers[state.selectedMarker];

      if (state.editingPauseSeconds) {
        if (step) {
          int v = (int)mk.pauseSeconds + step;
          v = constrain(v, 1, 99);
          mk.pauseSeconds = (uint8_t)v;
        }
        if (click) {
          state.editingPauseSeconds = false;
        }
      } else {
        if (step) markerMenuFocusMove(step);

        if (click) {
          switch (state.markerMenuFocus) {
            case MarkerMenuFocus::Movement:
              // cycle movement
              switch (mk.movement) {
                case MarkerMovement::None:      mk.movement = MarkerMovement::EaseIn; break;
                case MarkerMovement::EaseIn:    mk.movement = MarkerMovement::EaseOut; break;
                case MarkerMovement::EaseOut:   mk.movement = MarkerMovement::EaseInOut; break;
                case MarkerMovement::EaseInOut: mk.movement = MarkerMovement::None; break;
              }
              break;
            case MarkerMenuFocus::Pause:
              mk.pauseEnabled = !mk.pauseEnabled;
              // If we just disabled it, ensure focus doesn't get stuck on PauseSeconds
              if (!mk.pauseEnabled && state.markerMenuFocus == MarkerMenuFocus::PauseSeconds) {
                state.markerMenuFocus = MarkerMenuFocus::Back;
              }
              break;
            case MarkerMenuFocus::PauseSeconds:
              state.editingPauseSeconds = true;
              break;
            case MarkerMenuFocus::Back:
              state.screen = UiScreen::SetPosition;
              state.setPosFocus = SetPosFocus::Options;
              break;
          }
        }
      }
    } break;

    case UiScreen::Running: {
      // Minimal bring-up running screen: click returns
      if (click) {
        leaveToMain();
      }
    } break;
  }

  // -------------------
  // DRAW
  // -------------------
  ui.render(state);
  delay(16);
}