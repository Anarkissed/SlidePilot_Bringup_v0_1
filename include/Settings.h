#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct Settings {
  uint8_t markerCount = 2;   // 2..6
  bool    testingMode = false;
  bool    invertDir   = false;
};

void settingsLoad(Preferences& prefs, Settings& s);
void settingsSave(Preferences& prefs, const Settings& s);