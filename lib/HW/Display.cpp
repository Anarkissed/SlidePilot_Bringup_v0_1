#include "Display.h"

LGFX Display;

LGFX::LGFX() {
  {
    auto cfg = _bus.config();

    // 8-bit parallel bus
    cfg.freq_write = 20000000; // 20MHz is usually stable
    cfg.pin_wr  = 7;
    cfg.pin_rd  = -1;
    cfg.pin_rs  = 6;  // D/C

    // Data pins D0..D7
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

  {
    auto cfg = _panel.config();

    cfg.pin_cs   = 5;
    cfg.pin_rst  = 8;
    cfg.pin_busy = -1;

    cfg.panel_width  = 170;
    cfg.panel_height = 320;

    // You can tweak these if colors are inverted.
    cfg.invert = true;
    cfg.rgb_order = false;

    // Offsets (some ST7789 boards need these; 0 works for many)
    cfg.offset_x = 0;
    cfg.offset_y = 0;

    _panel.config(cfg);
  }

  // Backlight via PWM (still works if you just want on/off)
  {
    auto cfg = _light.config();
    cfg.pin_bl = PIN_TFT_BL;
    cfg.invert = false;
    cfg.freq   = 44100;
    cfg.pwm_channel = 7;
    _light.config(cfg);
    _panel.setLight(&_light);
  }

  setPanel(&_panel);
}
