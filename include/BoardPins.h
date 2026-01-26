#pragma once
#include <Arduino.h>

// ============================================================================
// ONE SOURCE OF TRUTH FOR PINS (per your rule)
// Board: LilyGO T-Display S3 (ESP32-S3) + your wiring ground-truth
// ============================================================================

// --- TFT power / backlight ---
// Battery note from LilyGO docs: GPIO15 must be HIGH for backlight when on battery.
static constexpr int PIN_TFT_POWER = 15;
static constexpr int PIN_TFT_BL    = 38;

// --- LCD I80 (8-bit parallel) pins (T-Display S3 ST7789) ---
static constexpr int PIN_LCD_WR = 8;
static constexpr int PIN_LCD_RD = 9;
static constexpr int PIN_LCD_DC = 7;

static constexpr int PIN_LCD_CS  = 6;
static constexpr int PIN_LCD_RST = 5;

static constexpr int PIN_LCD_D0 = 39;
static constexpr int PIN_LCD_D1 = 40;
static constexpr int PIN_LCD_D2 = 41;
static constexpr int PIN_LCD_D3 = 42;
static constexpr int PIN_LCD_D4 = 45;
static constexpr int PIN_LCD_D5 = 46;
static constexpr int PIN_LCD_D6 = 47;
static constexpr int PIN_LCD_D7 = 48;

// --- Buttons (your ground truth) ---
static constexpr int PIN_BTN_A = 0;
static constexpr int PIN_BTN_B = 14;

// --- Rotary encoder (your ground truth) ---
static constexpr int PIN_ENC_A  = 17;  // CLK
static constexpr int PIN_ENC_B  = 18;  // DT
static constexpr int PIN_ENC_SW = 1;   // SW

// --- AS5600 I2C (your ground truth) ---
static constexpr int PIN_I2C_SDA = 21;
static constexpr int PIN_I2C_SCL = 16;

// --- TMC2209 (your ground truth; motor stays disabled in Milestone 5) ---
static constexpr int PIN_TMC_STEP    = 13;
static constexpr int PIN_TMC_DIR     = 12;
static constexpr int PIN_TMC_EN      = 2;   // Active LOW
static constexpr int PIN_TMC_UART_TX = 10;
static constexpr int PIN_TMC_UART_RX = 11;