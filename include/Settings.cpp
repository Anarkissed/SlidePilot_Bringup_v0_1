#include <Arduino.h>
#include <Preferences.h>

#include "Config.h"
#include "Settings.h"

static Preferences g_prefs;

void settingsSetDefaults(Settings& s) {
  s.testingMode = false;
  s.invertDir   = false;
  s.lastMode    = 0;
  s.bootCount   = 0;
}

bool settingsLoad(Settings& s) {
  settingsSetDefaults(s);

  if (!g_prefs.begin(PREFS_NAMESPACE, false)) {
    // If prefs can't open, keep defaults.
    return false;
  }

  const uint16_t ver = g_prefs.getUShort(KEY_VER, 0);
  if (ver != SETTINGS_VERSION) {
    // Version mismatch: reset to defaults and write fresh.
    settingsSetDefaults(s);

    g_prefs.putUShort(KEY_VER, SETTINGS_VERSION);
    g_prefs.putBool(KEY_TEST, s.testingMode);
    g_prefs.putBool(KEY_INVERT, s.invertDir);
    g_prefs.putUChar(KEY_LASTMODE, s.lastMode);
    g_prefs.putUInt(KEY_BOOTCOUNT, s.bootCount);

    g_prefs.end();
    return true;
  }

  s.testingMode = g_prefs.getBool(KEY_TEST, s.testingMode);
  s.invertDir   = g_prefs.getBool(KEY_INVERT, s.invertDir);
  s.lastMode    = g_prefs.getUChar(KEY_LASTMODE, s.lastMode);
  s.bootCount   = g_prefs.getUInt(KEY_BOOTCOUNT, s.bootCount);

  g_prefs.end();
  return true;
}

bool settingsSave(const Settings& s) {
  if (!g_prefs.begin(PREFS_NAMESPACE, false)) {
    return false;
  }

  g_prefs.putUShort(KEY_VER, SETTINGS_VERSION);
  g_prefs.putBool(KEY_TEST, s.testingMode);
  g_prefs.putBool(KEY_INVERT, s.invertDir);
  g_prefs.putUChar(KEY_LASTMODE, s.lastMode);
  g_prefs.putUInt(KEY_BOOTCOUNT, s.bootCount);

  g_prefs.end();
  return true;
}