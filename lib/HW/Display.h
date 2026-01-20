#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

// LilyGO T-Display S3 (ST7789, 170x320) uses an 8-bit 8080 parallel bus.
// Pin mapping based on the common T-Display S3 pinout:
//   CS=6, DC=7, WR=8, RD=9, RST=5
//   D0=39 D1=40 D2=41 D3=42 D4=45 D5=46 D6=47 D7=48
//   BL=38, POWER_ON=15

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789   _panel;
  lgfx::Bus_Parallel8  _bus;
  lgfx::Light_PWM      _light;

public:
  LGFX() {
    // --- 8-bit parallel bus (i80/8080) ---
    {
      auto cfg = _bus.config();

      cfg.freq_write = 20000000;  // 20MHz is a safe starting point
      cfg.pin_wr = 8;
      cfg.pin_rd = 9;
      cfg.pin_rs = 7;             // D/C (aka RS)

      cfg.pin_d0 = 39;
      cfg.pin_d1 = 40;
      cfg.pin_d2 = 41;
      cfg.pin_d3 = 42;
      cfg.pin_d4 = 45;
      cfg.pin_d5 = 46;
      cfg.pin_d6 = 47;
      cfg.pin_d7 = 48;

      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    // --- Panel config ---
    {
      auto pcfg = _panel.config();
      pcfg.pin_cs   = 6;
      pcfg.pin_rst  = 5;
      pcfg.pin_busy = -1;

      pcfg.panel_width  = 170;
      pcfg.panel_height = 320;

      // The visible area is offset on this module.
      pcfg.offset_x = 35;
      pcfg.offset_y = 0;
      pcfg.offset_rotation = 0;

      pcfg.readable = false;
      pcfg.invert   = true;
      pcfg.rgb_order = false;
      pcfg.dlen_16bit = false;
      pcfg.bus_shared = false;

      _panel.config(pcfg);
    }

    // --- Backlight (PWM) ---
    {
      auto lcfg = _light.config();
      lcfg.pin_bl = 38;
      lcfg.invert = false;
      lcfg.freq   = 12000;
      lcfg.pwm_channel = 7;
      _light.config(lcfg);
      _panel.setLight(&_light);
    }

    setPanel(&_panel);
  }
};

// Header-only global display instance for bring-up.
// (Safe for this bring-up project which only has `src/main.cpp`.)
static LGFX Display;
