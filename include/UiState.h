#pragma once

#include <stdint.h>

// A small, self-contained UI state model for the bring-up firmware.
//
// This intentionally keeps "just enough" structure to:
//  - compile cleanly across translation units
//  - render the UI consistently
//  - preserve the key UX notes from the HTML mockups (header pill, equal padding, etc.)
//
// Expand this as the firmware grows (timeline editing, wizards, motor state, etc.).

enum class UiScreen : uint8_t {
  Main,
  TimelineWizard,
  SetPosition,
  Running,
  Settings,
};

enum class MainFocus : uint8_t {
  Back,
  Settings,
  Next,
};

enum class SettingsFocus : uint8_t {
  Exit,
  Brightness,
  MotorDir,
};

enum class PopupKind : uint8_t {
  None,
  CancelConfirm,
};

// Marker state for the timeline.
struct MarkerState {
  bool set = false;
  bool selected = false;  // which marker is currently selected / edited
};

struct UiState {
  // Global screen state
  UiScreen screen = UiScreen::Main;

  // Header shown in the "header pill" (top-left, always visible)
  const char* header = "SlidePilot";

  // Bottom nav focus
  MainFocus mainFocus = MainFocus::Settings;

  // Running screen
  bool isRunning = false;
  PopupKind popup = PopupKind::None;
  // UX note: default to NOT cancelling (highlight "No" first)
  bool cancelYesSelected = false;

  // Timeline markers (3 markers for bring-up)
  MarkerState markers[3] = {};
  uint8_t selectedMarker = 0;

  // Settings
  SettingsFocus settingsFocus = SettingsFocus::Exit;
  uint8_t brightness = 255;
  bool motorDirFlip = false;
};
