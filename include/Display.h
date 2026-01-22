#pragma once

#include <stdint.h>
#include <LovyanGFX.hpp>

#include "UiState.h"

class DisplayUI {
public:
  DisplayUI() = default;

  // Bind the LovyanGFX device and compute layout metrics.
  bool begin(lgfx::LGFX_Device* dev);

  // Render the current UI state.
  void render(const UiState& s);

private:
  lgfx::LGFX_Device* _lcd = nullptr;
  lgfx::LGFX_Sprite  _spr;

  struct UiMetrics {
    int W = 0;
    int H = 0;
    int pad = 0;
    int gap = 0;
    int topH = 0;
    int pillH = 0;
    int pillR = 0;
    int cardR = 0;
    int headerPillW = 0;
    int actionPillW = 0;
    int footerPillW = 0;
  };

  UiMetrics _ui;

  void computeMetrics();
  int  bottomBarTop() const;

  void textCenter(int x, int y, int w, int h, const char* txt, bool big, uint32_t col);
  void drawPill(int x, int y, int w, int h, const char* label, bool active, bool bigText, bool disabled = false);

  bool        focusIsMarker(const UiState& s, int* outIndex) const;
  const char* headerTitle(const UiState& s);
  const char* topRightLabel(const UiState& s);

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