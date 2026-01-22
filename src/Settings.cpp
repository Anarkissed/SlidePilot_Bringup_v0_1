#include "Settings.h"

static const char* kPrefsNS = "slidepilot";

void settingsLoad(Preferences& prefs, Settings& s) {
  // Open RO (true) to avoid accidental writes during load
  if (!prefs.begin(kPrefsNS, true)) return;

  s.markerCount = (uint8_t)prefs.getUChar("markerCount", s.markerCount);
  s.testingMode = prefs.getBool("testingMode", s.testingMode);
  s.invertDir   = prefs.getBool("invertDir", s.invertDir);

  prefs.end();

  // clamp markerCount 2..6
  if (s.markerCount < 2) s.markerCount = 2;
  if (s.markerCount > 6) s.markerCount = 6;
}

void settingsSave(Preferences& prefs, const Settings& s) {
  if (!prefs.begin(kPrefsNS, false)) return;

  prefs.putUChar("markerCount", s.markerCount);
  prefs.putBool("testingMode", s.testingMode);
  prefs.putBool("invertDir", s.invertDir);

  prefs.end();
}