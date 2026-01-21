#pragma once
#include <Arduino.h>

// Input timing
static constexpr uint32_t BTN_DEBOUNCE_MS     = 25;
static constexpr uint32_t ENC_SW_DEBOUNCE_MS  = 25;
static constexpr uint32_t INPUT_POLL_DELAY_MS = 2;

// Encoder detent handling (KY-040 often 4 transitions per detent)
static constexpr uint8_t ENC_COUNTS_PER_DETENT = 4;

// UI update rate (ms)
static constexpr uint32_t UI_UPDATE_MS = 33; // ~30 FPS (stable, reduces visible scan artifacts)