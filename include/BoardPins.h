#pragma once

// LilyGO T-Display S3 (AMOLED-less, ST7789 170x320) common pins
// If your board is wired differently, change these.

// Display power / backlight
static constexpr int PIN_TFT_POWER = 15;   // set HIGH to power TFT
static constexpr int PIN_TFT_BL    = 38;   // backlight (HIGH=on)

// Two onboard buttons (often labeled 0 and 14 on T-Display S3)
static constexpr int PIN_BTN_A = 0;
static constexpr int PIN_BTN_B = 14;
