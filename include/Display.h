#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "Settings.h"
#include "MarkerIcons.h"

// Screens (mirror HTML simulation intent)
enum class UiScreen : uint8_t {
  MAIN,
  SETTINGS,
  SET_POS,
  WIZARD,
  RUN,
  POPUP
};

enum class PopupKind : uint8_t {
  NONE,
  CONFIRM_CANCEL,
  NEED_ALL_MARKERS
};

// MAIN focus order (cyclic):
// Settings -> Marker0..MarkerN-1 -> Next -> Mode -> Settings
enum class MainFocus : uint8_t {
  SETTINGS = 0,

  MARKER0,
  MARKER1,
  MARKER2,
  MARKER3,
  MARKER4,
  MARKER5,
  MARKER6,
  MARKER7,

  NEXT,   // bottom-left (swapped)
  MODE    // bottom-right (swapped)
};

enum class SettingsFocus : uint8_t {
  BACK,
  TESTING_MODE,
  INVERT_DIR
};

struct As5600View {
  bool present = false;
  bool readOk = false;
  uint16_t raw = 0;        // 0..4095
  int32_t ticks = 0;       // multi-turn ticks
  float deg = 0.0f;        // 0..360
};

struct UiState {
  UiScreen screen = UiScreen::MAIN;

  // MAIN
  MainFocus mainFocus = MainFocus::MARKER0;

  static constexpr int kMaxMarkers = 8;
  int markerCount = 2; // start with 2 markers like your current UI
  MarkerState markers[kMaxMarkers];

  // If true, markers have all been configured and Next is enabled
  bool allMarkersSet = false;

  // Mode name shown in header pill (Single/Bounce/Timelapse)
  const char* modeName = "Single";

  // SETTINGS
  SettingsFocus settingsFocus = SettingsFocus::TESTING_MODE;

  // POPUP
  PopupKind popup = PopupKind::NONE;
  bool popupYesSelected = false;  // default false => "No" highlighted

  // Data
  Settings settings;
  As5600View as5600;
  uint16_t fps = 0;

  // Live inputs (optional debug)
  bool btnA = false;
  bool btnB = false;
  bool encSw = false;
};

class DisplayUI {
 public:
  bool begin(lgfx::LGFX_Device* dev);
  void render(const UiState& s);

 private:
  lgfx::LGFX_Device* _lcd = nullptr;
  lgfx::LGFX_Device* _gfx = nullptr;

  struct UiMetrics {
    int W = 0, H = 0;
    int pad = 0, gap = 0;
    int topH = 0;
    int pillH = 0;
    int pillR = 0;
    int headerPillW = 0;
    int actionPillW = 0;
    int footerPillW = 0;
    int cardR = 0;
  } _ui;

  // Theme colors (match HTML)
  static uint32_t C_BG(lgfx::LGFX_Device* lcd)      { return lcd->color888( 15,  22,  32); }
  static uint32_t C_SURF(lgfx::LGFX_Device* lcd)    { return lcd->color888( 27,  38,  51); }
  static uint32_t C_PILL(lgfx::LGFX_Device* lcd)    { return lcd->color888( 38,  53,  71); }
  static uint32_t C_ACCENT(lgfx::LGFX_Device* lcd)  { return lcd->color888( 73, 182, 255); }
  static uint32_t C_TEXT(lgfx::LGFX_Device* lcd)    { return lcd->color888(232, 238, 247); }
  static uint32_t C_DIM(lgfx::LGFX_Device* lcd)     { return lcd->color888(169, 183, 199); }
  static uint32_t C_DISABLED(lgfx::LGFX_Device* lcd){ return lcd->color888( 24,  34,  46); }

  void computeMetrics();

  void textCenter(int x, int y, int w, int h, const char* txt, bool big, uint32_t col);
  void drawPill(int x, int y, int w, int h, const char* label, bool active, bool bigText = true,
                bool disabled = false);

  const char* headerTitle(const UiState& s);
  const char* topRightLabel(const UiState& s);

  int bottomBarTop() const;

  bool focusIsMarker(const UiState& s, int* outIndex = nullptr) const;

  void drawTopBar(const UiState& s);
  void drawFooterMain(const UiState& s);
  void drawFooterBackEnter(const UiState& s, const char* rightLabel);

  void drawMain(const UiState& s);
  void drawSettings(const UiState& s);
  void drawSetPos(const UiState& s);
  void drawWizard(const UiState& s);
  void drawRun(const UiState& s);
  void drawPopup(const UiState& s);
};