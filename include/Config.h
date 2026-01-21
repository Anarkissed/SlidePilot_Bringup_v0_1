#pragma once
#include <stdint.h>

// ---------------------------
// Input timing
// ---------------------------
static constexpr uint32_t BTN_DEBOUNCE_MS      = 25;
static constexpr uint32_t ENC_SW_DEBOUNCE_MS   = 25;

// Encoder characteristics (KY-040 typically 4 transitions per detent)
static constexpr uint8_t  ENC_COUNTS_PER_DETENT = 4;

// Loop + UI timing
static constexpr uint32_t UI_UPDATE_MS         = 33; // ~30 FPS UI refresh
static constexpr uint32_t INPUT_POLL_DELAY_MS  = 2;  // fast polling for encoder

// ---------------------------
// Preferences / NVS
// ---------------------------
static constexpr const char* PREFS_NAMESPACE = "slidepilot";

// Versioning (lets us reset defaults safely if struct evolves)
static constexpr uint16_t SETTINGS_VERSION = 1;
static constexpr const char* KEY_VER       = "ver";
static constexpr const char* KEY_TEST      = "test";
static constexpr const char* KEY_INVERT    = "inv";
static constexpr const char* KEY_LASTMODE  = "mode";
static constexpr const char* KEY_BOOTCOUNT = "boots";

// Simple bounds for demo
static constexpr uint8_t LASTMODE_MIN = 0;
static constexpr uint8_t LASTMODE_MAX = 4;

// UI toast duration
static constexpr uint32_t SAVED_TOAST_MS = 800;