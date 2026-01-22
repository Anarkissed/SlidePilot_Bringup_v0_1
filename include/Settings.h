#pragma once
#include <Arduino.h>

struct Settings {
  bool testingMode = false;
  bool invertDir   = false;
  uint8_t lastMode = 0;
  uint32_t bootCount = 0;

  // versioning
  uint16_t version = 1;
};

void settingsLoad(Settings& s);
void settingsSave(const Settings& s);