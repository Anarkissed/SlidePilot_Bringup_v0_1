#pragma once

#include <stdint.h>

// -----------------------------------------------------------------------------
// UI State Model (Bring-up → Real UI scaffold)
//
// This is intentionally lightweight but structured so we can:
//  - Render the UI exactly like the HTML mockups
//  - Drive navigation from a rotary encoder + click + back button
//  - Support up to 6 markers, evenly spaced on the timeline
// -----------------------------------------------------------------------------

// Screens
enum class UiScreen : uint8_t {
  Main,
  Settings,
  SetPosition,
  MarkerMenu,
  Running,
};

// Modes
enum class SlideMode : uint8_t {
  Single,
  Bounce,
  Timelapse,
};

// Focus order on MAIN screen
// Settings → marker1..marker6 → Mode → Next → wrap
enum class MainFocus : uint8_t {
  Settings,
  Marker1,
  Marker2,
  Marker3,
  Marker4,
  Marker5,
  Marker6,
  Mode,
  Next,
};

// Set Position screen focus
enum class SetPosFocus : uint8_t {
  Center,   // camera + triangle + timeline area
  Options,  // bottom-left
  Back,     // bottom-right (saves)
};

// Marker menu focus
enum class MarkerMenuFocus : uint8_t {
  Movement,
  Pause,
  PauseSeconds,
  Back,
};

enum class MarkerMovement : uint8_t {
  None,
  EaseIn,
  EaseOut,
  EaseInOut,
};

enum class PopupKind : uint8_t {
  None,
  CancelConfirm,
};

// Marker state
struct MarkerState {
  bool set = false;
  uint8_t posPercent = 0;      // 0..100
  MarkerMovement movement = MarkerMovement::None;
  bool pauseEnabled = false;
  uint8_t pauseSeconds = 1;    // 1..99 (clamped)
};

struct UiState {
  // Screen state
  UiScreen screen = UiScreen::Main;

  // Mode (top-left header pill)
  SlideMode mode = SlideMode::Single;

  // MAIN screen focus
  MainFocus mainFocus = MainFocus::Settings;

  // Markers
  static constexpr uint8_t kMaxMarkers = 6;
  MarkerState markers[kMaxMarkers] = {};
  uint8_t markerCount = 2;      // how many marker slots are active (<=6)
  uint8_t selectedMarker = 0;   // 0..markerCount-1

  // SETTINGS screen
  uint8_t settingsSelection = 0; // 0=Add Marker, 1=Exit

  // SET POSITION screen
  SetPosFocus setPosFocus = SetPosFocus::Center;

  // Marker menu
  MarkerMenuFocus markerMenuFocus = MarkerMenuFocus::Movement;
  bool editingPauseSeconds = false;

  // Running screen
  PopupKind popup = PopupKind::None;
  bool cancelYesSelected = false;
};