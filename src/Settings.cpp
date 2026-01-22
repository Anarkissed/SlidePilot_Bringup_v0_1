#include "Settings.h"
#include <Preferences.h>

static const char* kNs = "slidepilot";

void settingsLoad(Settings& s) {
  Preferences prefs;
  if (!prefs.begin(kNs, true)) {
    // defaults
    s = Settings{};
    return;
  }

  const uint16_t ver = prefs.getUShort("ver", 1);
  s.version = ver;

  s.testingMode = prefs.getBool("test", false);
  s.invertDir   = prefs.getBool("inv", false);
  s.lastMode    = prefs.getUChar("mode", 0);
  s.bootCount   = prefs.getULong("boot", 0);

  prefs.end();
}

void settingsSave(const Settings& s) {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) return;

  prefs.putUShort("ver", s.version);
  prefs.putBool("test", s.testingMode);
  prefs.putBool("inv",  s.invertDir);
  prefs.putUChar("mode", s.lastMode);
  prefs.putULong("boot", s.bootCount);

  prefs.end();
}