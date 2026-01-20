#pragma once

// LilyGO T-Display S3 (170x320 ST7789, 8-bit parallel bus)
// Pin mapping based on the public T-Display S3 pinout/schematic.
// If your board revision differs, change these values.

// Power enable (T-Display S3): set HIGH to power the TFT
#ifndef PIN_LCD_POWER
#define PIN_LCD_POWER 15
#endif

// Backlight enable/PWM pin
#ifndef PIN_LCD_BL
#define PIN_LCD_BL 38
#endif

// Buttons on the T-Display S3 board
#ifndef PIN_BTN_A
#define PIN_BTN_A 0
#endif

#ifndef PIN_BTN_B
#define PIN_BTN_B 14
#endif
