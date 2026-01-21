#pragma once
#include <stdint.h>

// Single source of truth for persisted settings.
struct Settings {
  bool     testingMode = false;
  bool     invertDir   = false;
  uint8_t  lastMode    = 0;
  uint32_t bootCount   = 0;
};

// Defaults (used on first boot or version mismatch)
void settingsSetDefaults(Settings& s);

// Load from Preferences (NVS). Returns true if loaded (or defaults applied).
bool settingsLoad(Settings& s);

// Save to Preferences (NVS). Returns true if save succeeded.
bool settingsSave(const Settings& s);