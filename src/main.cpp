// src/main.cpp
// SlidePilot — Display bring-up + real UI renderer
// Goal: keep TFT power + backlight ON, and render the existing DisplayUI.

#include <Arduino.h>
#include <LovyanGFX.hpp>

#include "BoardPins.h"
#include "Display.h"

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

      // ESP32-S3 parallel (I80) bus port index
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

      // T-Display S3 offsets for the 170x320 ST7789
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
static DisplayUI g_display;
static UiState   g_state;

static void powerOnDisplay() {
  // TFT power rail
  pinMode(PIN_TFT_POWER, OUTPUT);
  digitalWrite(PIN_TFT_POWER, HIGH);

  // Force backlight ON immediately (PWM takes over after lcd.init)
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
}

void setup() {
  delay(50);

  // Ensure the display rail + backlight are on before init
  powerOnDisplay();
  delay(50);

  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);

  // Hand the working lcd instance to the real UI
  g_display.begin(&lcd);

  // Default UI state for bring-up.
  g_state = UiState{};
  g_state.header = "Timeline Wizard";
  g_state.primaryPill = "Back";
  g_state.secondaryPill = "Settings";
  g_state.tertiaryPill = "Next";
  g_state.markerSelected = 0;
  g_state.markers[0].isSet = true;
  g_state.markers[1].isSet = false;
  g_state.markers[2].isSet = false;
}

void loop() {
  // Keep brightness pinned in case anything else tries to dim it
  lcd.setBrightness(255);

  // Render your real UI
  g_display.render(g_state);

  delay(16); // ~60 FPS
}