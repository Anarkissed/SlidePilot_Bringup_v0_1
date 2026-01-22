#pragma once

// LilyGO T-Display S3 (ESP32-S3) pin map
// Display is ST7789 on 8-bit parallel bus (NOT SPI)

static constexpr int PIN_TFT_POWER = 15;   // set HIGH to power TFT
static constexpr int PIN_TFT_BL    = 38;   // backlight (PWM, HIGH=on)

// LCD parallel bus pins
static constexpr int PIN_LCD_D0 = 39;
static constexpr int PIN_LCD_D1 = 40;
static constexpr int PIN_LCD_D2 = 41;
static constexpr int PIN_LCD_D3 = 42;
static constexpr int PIN_LCD_D4 = 45;
static constexpr int PIN_LCD_D5 = 46;
static constexpr int PIN_LCD_D6 = 47;
static constexpr int PIN_LCD_D7 = 48;

static constexpr int PIN_LCD_WR  = 8;
static constexpr int PIN_LCD_RD  = 9;
static constexpr int PIN_LCD_DC  = 7;
static constexpr int PIN_LCD_CS  = 6;
static constexpr int PIN_LCD_RST = 5;

// Onboard buttons (ground truth)
static constexpr int PIN_BTN_A = 0;   // Button A (BOOT)
static constexpr int PIN_BTN_B = 14;  // Button B

// TMC2209 (ground truth)
static constexpr int PIN_TMC_STEP    = 13;
static constexpr int PIN_TMC_DIR     = 12;
static constexpr int PIN_TMC_EN      = 2;   // active LOW
static constexpr int PIN_TMC_UART_TX = 10;  // ESP -> driver RX
static constexpr int PIN_TMC_UART_RX = 11;  // ESP <- driver TX

// AS5600 I2C (ground truth)
static constexpr int PIN_I2C_SDA = 21;
static constexpr int PIN_I2C_SCL = 16;

// Rotary encoder (ground truth)
static constexpr int PIN_ENC_A  = 17; // KY-040 CLK
static constexpr int PIN_ENC_B  = 18; // KY-040 DT
static constexpr int PIN_ENC_SW = 1;  // KY-040 SW

// Compatibility aliases (older code used different names)
static constexpr int PIN_TFT_PWR = PIN_TFT_POWER;
static constexpr int PIN_ENC_CLK = PIN_ENC_A;
static constexpr int PIN_ENC_DT  = PIN_ENC_B;