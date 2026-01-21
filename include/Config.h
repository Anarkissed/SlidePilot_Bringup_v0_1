#pragma once

// Debounce timing
static constexpr uint32_t BTN_DEBOUNCE_MS      = 25;
static constexpr uint32_t ENC_SW_DEBOUNCE_MS   = 25;

// Encoder characteristics (KY-040 typically 4 transitions per detent)
static constexpr uint8_t  ENC_COUNTS_PER_DETENT = 4;

// Loop + UI timing
static constexpr uint32_t UI_UPDATE_MS         = 33; // ~30 FPS UI refresh
static constexpr uint32_t INPUT_POLL_DELAY_MS  = 2;  // fast polling for encoder