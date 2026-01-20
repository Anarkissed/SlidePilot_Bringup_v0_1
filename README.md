# SlidePilot Bring-up v0.1

This is a **clean minimal PlatformIO project** for the ESP32-S3 + LilyGO T-Display S3.

What it does:
- Powers the TFT
- Initializes LovyanGFX display
- Prints text
- Reads the two onboard buttons and displays their state

## Build / Upload

In VS Code + PlatformIO:
- Select environment **tdisplay_s3**
- Build / Upload

From terminal:
- `pio run -e tdisplay_s3`
- `pio run -e tdisplay_s3 -t upload`
- `pio device monitor -b 115200`

## Pins
Edit `include/BoardPins.h` if your board differs.

## Notes
If the screen is black:
1) Confirm `PIN_TFT_POWER` is correct (set HIGH)
2) Confirm backlight pin (`PIN_TFT_BL`)
3) Try changing `Display.setRotation(1)` to `0/2/3`
4) If colors look inverted, flip `_panel.config().invert`
