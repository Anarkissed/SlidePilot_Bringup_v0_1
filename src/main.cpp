#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>

// IMPORTANT: Use the ONE pin header for this project.
#include "BoardPins.h"

// ------------------------------------------------------------
// LovyanGFX display driver for LilyGO T-Display S3
// ------------------------------------------------------------
// This board uses an ST7789 panel driven over an 8-bit 8080 (I80) parallel bus.
// Pins are defined in include/BoardPins.h.
class LGFX_TDisplayS3 : public lgfx::LGFX_Device {
 public:
  lgfx::Panel_ST7789   _panel;
  lgfx::Bus_Parallel8  _bus;
  lgfx::Light_PWM      _light;

  LGFX_TDisplayS3() {
    // ---- 8-bit Parallel Bus (I80) ----
    {
      auto cfg = _bus.config();

      // Write / read clock
      cfg.freq_write = 20000000;  // 20MHz is safe for bring-up
      cfg.freq_read  = 16000000;

      // 8080 control pins
      cfg.pin_wr = PIN_LCD_WR;
      cfg.pin_rd = PIN_LCD_RD;
      cfg.pin_rs = PIN_LCD_DC;  // RS (a.k.a. DC)

      // Data bus D0..D7
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

      // T-Display S3 uses 170x320 ST7789
      cfg.panel_width  = 170;
      cfg.panel_height = 320;

      // Most T-Display S3 variants need an X offset because the ST7789 memory is 240 wide.
      // This value is the common working offset for 170x320 panels.
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
// AS5600 helpers
// ------------------------------------------------------------
static constexpr uint8_t AS5600_ADDR = 0x36;

static bool i2cDevicePresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

static bool as5600ReadRawAngle(uint16_t &rawOut) {
  // AS5600 RAW ANGLE registers: 0x0C (MSB), 0x0D (LSB)
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(0x0C);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t n = Wire.requestFrom((int)AS5600_ADDR, 2);
  if (n != 2) {
    return false;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  rawOut = ((uint16_t)msb << 8) | lsb;
  rawOut &= 0x0FFF;  // 12-bit
  return true;
}

static float rawToDegrees(uint16_t raw) {
  return (raw * 360.0f) / 4096.0f;
}

// ------------------------------------------------------------
// UI drawing
// ------------------------------------------------------------
static void flashTestPattern() {
  lcd.fillScreen(lcd.color888(255, 0, 0));
  delay(150);
  lcd.fillScreen(lcd.color888(0, 255, 0));
  delay(150);
  lcd.fillScreen(lcd.color888(0, 0, 255));
  delay(150);
  lcd.fillScreen(lcd.color888(0, 0, 0));
  delay(150);
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

  lcd.setCursor(8, 52);
  lcd.print("I2C: SDA=GPIO21  SCL=GPIO16");

  // Divider line
  lcd.drawFastHLine(0, 68, lcd.width(), lcd.color888(60, 60, 60));
}

static void drawLiveStatus(bool btnA, bool btnB, bool i2cOk, bool as5600Ok, uint16_t raw, float deg) {
  // Clear live status area
  lcd.fillRect(0, 72, lcd.width(), lcd.height() - 72, lcd.color888(0, 0, 0));

  lcd.setTextSize(2);
  lcd.setCursor(8, 80);
  lcd.setTextColor(lcd.color888(255, 255, 0));
  lcd.printf("A: %s   B: %s", btnA ? "PRESSED" : "-----", btnB ? "PRESSED" : "-----");

  lcd.setTextSize(1);
  lcd.setTextColor(lcd.color888(180, 180, 180));
  lcd.setCursor(8, 112);
  lcd.printf("I2C bus: %s", i2cOk ? "OK" : "FAIL");

  lcd.setCursor(8, 128);
  if (!as5600Ok) {
    lcd.print("AS5600: NOT FOUND @0x36");
  } else {
    lcd.printf("AS5600: raw=0x%03X  deg=%0.2f", raw, deg);
  }

  lcd.setCursor(8, 148);
  lcd.print("(This is hardware bring-up only: no motor motion)");
}

void setup() {
  Serial.begin(115200);
  delay(50);

  // --- Motor safety (NO motion / NO holding torque during bring-up) ---
  // If your TMC2209 EN pin is wired to GPIO2 (recommended), this will disable the driver.
  // If EN is hard-tied to GND, firmware cannot disable the driver — you must rewire EN.
  pinMode(PIN_TMC_STEP, OUTPUT);
  digitalWrite(PIN_TMC_STEP, LOW);

  pinMode(PIN_TMC_DIR, OUTPUT);
  digitalWrite(PIN_TMC_DIR, LOW);

  pinMode(PIN_TMC_EN, OUTPUT);
  digitalWrite(PIN_TMC_EN, HIGH); // Active-LOW enable => HIGH disables

  // Keep UART pins quiet during bring-up
  pinMode(PIN_TMC_UART_TX, INPUT);
  pinMode(PIN_TMC_UART_RX, INPUT);

  // --- Power + backlight pins (must be set before lcd.init on this board) ---
  pinMode(PIN_TFT_POWER, OUTPUT);
  digitalWrite(PIN_TFT_POWER, HIGH);

  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  // --- Buttons ---
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);

  // --- I2C ---
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  // --- LCD ---
  lcd.init();
  lcd.setRotation(1);     // landscape
  lcd.setBrightness(255); // 0..255

  flashTestPattern();
  drawStaticLayout();
}

void loop() {
  const bool btnA_pressed = (digitalRead(PIN_BTN_A) == LOW);
  const bool btnB_pressed = (digitalRead(PIN_BTN_B) == LOW);

  const bool i2c_ok = true; // Wire is initialized; treat as OK unless bus init fails
  const bool as5600_present = i2cDevicePresent(AS5600_ADDR);

  uint16_t raw = 0;
  float deg = 0.0f;
  bool as5600_ok = false;

  if (as5600_present) {
    as5600_ok = as5600ReadRawAngle(raw);
    if (as5600_ok) {
      deg = rawToDegrees(raw);
    }
  }

  drawLiveStatus(btnA_pressed, btnB_pressed, i2c_ok, as5600_present && as5600_ok, raw, deg);

  delay(100);
}