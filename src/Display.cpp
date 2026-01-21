#include "Display.h"

static void logSpriteStatus(bool useSprite, int depth, bool psram) {
  Serial.printf("[DisplayUI] sprite=%s depth=%d psram=%s\n",
                useSprite ? "ON" : "OFF",
                depth,
                psram ? "true" : "false");
}

bool DisplayUI::begin(lgfx::LGFX_Device* dev) {
  _lcd = dev;
  if (!_lcd) return false;

  computeMetrics();
  _gfx = _lcd;

  // Sprite usage currently disabled (stable + no tearing work yet)
  logSpriteStatus(false, 0, false);
  return true;
}

void DisplayUI::computeMetrics() {
  _ui.W = (int)_lcd->width();
  _ui.H = (int)_lcd->height();

  // HTML canvas reference: W=480 H=270
  const float sx = (float)_ui.W / 480.0f;
  const float sy = (float)_ui.H / 270.0f;

  auto scX = [&](int v) { return (int)lroundf(v * sx); };
  auto scY = [&](int v) { return (int)lroundf(v * sy); };
  auto scR = [&](int v) { return (int)lroundf(v * (sx + sy) * 0.5f); };

  _ui.pad         = scX(14);
  _ui.gap         = scY(12);
  _ui.topH        = scY(38);
  _ui.pillH       = scY(38);
  _ui.pillR       = scR(19);
  _ui.cardR       = scR(18);
  _ui.headerPillW = (int)lroundf(_ui.W * 0.50f);
  _ui.actionPillW = scX(160);
  _ui.footerPillW = scX(160);

  // Safety clamps
  if (_ui.pad < 8) _ui.pad = 8;
  if (_ui.gap < 6) _ui.gap = 6;
  if (_ui.pillH < 22) _ui.pillH = 22;
  if (_ui.pillR < 10) _ui.pillR = 10;
  if (_ui.cardR < 10) _ui.cardR = 10;
}

int DisplayUI::bottomBarTop() const {
  return _ui.H - _ui.pad - _ui.pillH;
}

void DisplayUI::textCenter(int x, int y, int w, int h, const char* txt, bool big, uint32_t col) {
  _gfx->setTextColor(col);
  _gfx->setTextDatum(textdatum_t::middle_center);
  _gfx->setTextSize(big ? 2 : 1);
  _gfx->drawString(txt, x + w/2, y + h/2);
}

void DisplayUI::drawPill(int x, int y, int w, int h, const char* label, bool active, bool bigText, bool disabled) {
  uint32_t fill;
  uint32_t txt;
  if (disabled) {
    fill = C_DISABLED(_lcd);
    txt  = C_DIM(_lcd);
  } else {
    fill = active ? C_ACCENT(_lcd) : C_PILL(_lcd);
    txt  = C_TEXT(_lcd);
  }
  _gfx->fillRoundRect(x, y, w, h, _ui.pillR, fill);
  textCenter(x, y, w, h, label, bigText, txt);
}

bool DisplayUI::focusIsMarker(const UiState& s, int* outIndex) const {
  // MARKER0..MARKER7 contiguous
  const int f = (int)s.mainFocus;
  const int base = (int)MainFocus::MARKER0;
  const int last = (int)MainFocus::MARKER7;
  if (f >= base && f <= last) {
    const int idx = f - base;
    if (outIndex) *outIndex = idx;
    return idx >= 0 && idx < s.markerCount;
  }
  return false;
}

const char* DisplayUI::headerTitle(const UiState& s) {
  switch (s.screen) {
    case UiScreen::MAIN:     return s.modeName ? s.modeName : "Mode";
    case UiScreen::SETTINGS: return "Settings";
    case UiScreen::SET_POS:  return "Set Position";
    case UiScreen::WIZARD:   return "Timeline Wizard";
    case UiScreen::RUN:      return "Running";
    case UiScreen::POPUP:    return "Running";
    default:                 return "SlidePilot";
  }
}

const char* DisplayUI::topRightLabel(const UiState& s) {
  switch (s.screen) {
    case UiScreen::MAIN:     return "Settings";
    case UiScreen::SETTINGS: return "Back";
    case UiScreen::SET_POS:  return "Back";
    case UiScreen::WIZARD:   return "Back";
    case UiScreen::RUN:      return "Cancel";
    case UiScreen::POPUP:    return "Cancel";
    default:                 return "";
  }
}

void DisplayUI::drawTopBar(const UiState& s) {
  const int x = _ui.pad;
  const int y = _ui.pad;

  drawPill(x, y, _ui.headerPillW, _ui.pillH, headerTitle(s), false, true);

  const int rx = _ui.W - _ui.pad - _ui.actionPillW;

  bool rightActive = false;
  if (s.screen == UiScreen::MAIN) rightActive = (s.mainFocus == MainFocus::SETTINGS);

  drawPill(rx, y, _ui.actionPillW, _ui.pillH, topRightLabel(s), rightActive, true);
}

void DisplayUI::drawFooterMain(const UiState& s) {
  const int y = bottomBarTop();
  const int leftX  = _ui.pad;
  const int rightX = _ui.W - _ui.pad - _ui.footerPillW;

  // SWAPPED: left=Next, right=Mode
  const bool nextActive = (s.mainFocus == MainFocus::NEXT);
  const bool modeActive = (s.mainFocus == MainFocus::MODE);

  const bool nextDisabled = !s.allMarkersSet;

  drawPill(leftX,  y, _ui.footerPillW, _ui.pillH, "Next", nextActive, true, nextDisabled);
  drawPill(rightX, y, _ui.footerPillW, _ui.pillH, "Mode", modeActive, true, false);
}

void DisplayUI::drawFooterBackEnter(const UiState& s, const char* rightLabel) {
  const int y = bottomBarTop();
  const int leftX  = _ui.pad;
  const int rightX = _ui.W - _ui.pad - _ui.footerPillW;

  bool leftActive = false;
  if (s.screen == UiScreen::SETTINGS) {
    leftActive = (s.settingsFocus == SettingsFocus::BACK);
  }

  drawPill(leftX,  y, _ui.footerPillW, _ui.pillH, "Back", leftActive, true);
  drawPill(rightX, y, _ui.footerPillW, _ui.pillH, rightLabel ? rightLabel : "", false, true);
}

void DisplayUI::drawMain(const UiState& s) {
  const int cardY = _ui.pad + _ui.topH + _ui.gap;
  const int cardH = (bottomBarTop() - _ui.gap) - cardY;
  const int cardX = _ui.pad;
  const int cardW = _ui.W - _ui.pad*2;

  // Highlight center squircle when focus is on timeline markers
  int selectedMarkerIdx = -1;
  const bool markerFocused = focusIsMarker(s, &selectedMarkerIdx);

  _gfx->fillRoundRect(cardX, cardY, cardW, cardH, _ui.cardR, C_SURF(_lcd));
  if (markerFocused) {
    _gfx->drawRoundRect(cardX, cardY, cardW, cardH, _ui.cardR, C_ACCENT(_lcd));
    _gfx->drawRoundRect(cardX+1, cardY+1, cardW-2, cardH-2, _ui.cardR, C_ACCENT(_lcd));
  }

  // Timeline baseline
  const int innerX = cardX + 18;
  const int innerW = cardW - 36;

  const int railY = cardY + (int)lroundf(cardH * 0.68f);
  const int railX1 = innerX;
  const int railX2 = innerX + innerW;

  _gfx->drawFastHLine(railX1, railY, railX2 - railX1, C_DIM(_lcd));

  // Place markers evenly left->right
  auto markerX = [&](int idx)->int {
    if (s.markerCount <= 1) return (railX1 + railX2)/2;
    const float t = (float)idx / (float)(s.markerCount - 1);
    return railX1 + (int)lroundf((railX2 - railX1) * t);
  };

  const int iconR = 14;

  // Draw markers with icon states
  for (int i = 0; i < s.markerCount; i++) {
    const int mx = markerX(i);
    const bool isSel = markerFocused && (i == selectedMarkerIdx);

    const MarkerIconKind k = markerIconFromState(s.markers[i]);
    drawMarkerIcon(_gfx, mx, railY, iconR, k, isSel,
                   C_PILL(_lcd), C_TEXT(_lcd), C_ACCENT(_lcd), C_DIM(_lcd));
  }

  // Camera + arrow ONLY when user is selecting timeline points
  if (markerFocused && selectedMarkerIdx >= 0) {
    const int selX = markerX(selectedMarkerIdx);

    // Padding stack: padding > camera > padding > arrow > padding > marker/timeline
    const int camW = 34;
    const int camH = 22;

    const int camY = railY - (iconR + 16 + camH); // sits above arrow with padding
    drawCameraIcon(_gfx, selX, camY, camW, camH, C_TEXT(_lcd), C_PILL(_lcd));

    const int arrowTop = camY + camH/2 + 10;
    const int arrowBottom = railY - iconR - 6;
    drawDownArrow(_gfx, selX, arrowTop, arrowBottom, C_DIM(_lcd));
  }

  // IMPORTANT: remove debug text inside main squircle (per your request)
}

void DisplayUI::drawSettings(const UiState& s) {
  const int cardY = _ui.pad + _ui.topH + _ui.gap;
  const int cardH = (bottomBarTop() - _ui.gap) - cardY;
  _gfx->fillRoundRect(_ui.pad, cardY, _ui.W - _ui.pad*2, cardH, _ui.cardR, C_SURF(_lcd));

  const int x = _ui.pad + 14;
  int y = cardY + 16;
  const int rowW = _ui.W - (_ui.pad + 14)*2;

  auto row = [&](const char* label, const char* value, bool active) {
    _gfx->fillRoundRect(x, y, rowW, 26, 10, active ? C_ACCENT(_lcd) : C_PILL(_lcd));
    _gfx->setTextDatum(textdatum_t::middle_left);
    _gfx->setTextSize(1);
    _gfx->setTextColor(C_TEXT(_lcd));
    _gfx->drawString(label, x + 10, y + 13);
    _gfx->setTextDatum(textdatum_t::middle_right);
    _gfx->drawString(value, x + rowW - 10, y + 13);
    y += 34;
  };

  row("Testing Mode", s.settings.testingMode ? "ON" : "OFF",
      s.settingsFocus == SettingsFocus::TESTING_MODE);

  row("Invert Direction", s.settings.invertDir ? "ON" : "OFF",
      s.settingsFocus == SettingsFocus::INVERT_DIR);

  drawFooterBackEnter(s, "");
}

void DisplayUI::drawSetPos(const UiState& s) {
  const int cardY = _ui.pad + _ui.topH + _ui.gap;
  const int cardH = (bottomBarTop() - _ui.gap) - cardY;
  _gfx->fillRoundRect(_ui.pad, cardY, _ui.W - _ui.pad*2, cardH, _ui.cardR, C_SURF(_lcd));

  _gfx->setTextDatum(textdatum_t::top_left);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_TEXT(_lcd));
  _gfx->setCursor(_ui.pad + 16, cardY + 18);
  _gfx->print("Set Position");

  drawFooterBackEnter(s, "Save");
}

void DisplayUI::drawWizard(const UiState& s) {
  const int cardY = _ui.pad + _ui.topH + _ui.gap;
  const int cardH = (bottomBarTop() - _ui.gap) - cardY;
  _gfx->fillRoundRect(_ui.pad, cardY, _ui.W - _ui.pad*2, cardH, _ui.cardR, C_SURF(_lcd));

  _gfx->setTextDatum(textdatum_t::top_left);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_TEXT(_lcd));
  _gfx->setCursor(_ui.pad + 16, cardY + 18);
  _gfx->print("Timeline Wizard");

  drawFooterBackEnter(s, "Next");
}

void DisplayUI::drawRun(const UiState& s) {
  const int cardY = _ui.pad + _ui.topH + _ui.gap;
  const int cardH = (bottomBarTop() - _ui.gap) - cardY;
  _gfx->fillRoundRect(_ui.pad, cardY, _ui.W - _ui.pad*2, cardH, _ui.cardR, C_SURF(_lcd));

  _gfx->setTextDatum(textdatum_t::top_left);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_TEXT(_lcd));
  _gfx->setCursor(_ui.pad + 16, cardY + 18);
  _gfx->print("Running");

  _gfx->setTextSize(1);
  _gfx->setCursor(_ui.pad + 16, cardY + 52);
  _gfx->print("Motor DISABLED.");
}

void DisplayUI::drawPopup(const UiState& s) {
  const int w = _ui.W - _ui.pad*4;
  const int h = 96;
  const int x = _ui.pad*2;
  const int y = (_ui.H - h) / 2;

  _gfx->fillRoundRect(x, y, w, h, 16, C_SURF(_lcd));
  _gfx->setTextDatum(textdatum_t::top_left);
  _gfx->setTextSize(1);
  _gfx->setTextColor(C_TEXT(_lcd));
  _gfx->setCursor(x + 14, y + 14);

  if (s.popup == PopupKind::NEED_ALL_MARKERS) {
    _gfx->print("Set all markers first.");
  } else {
    _gfx->print("Cancel the run?");
  }

  const int by = y + 50;
  const int bw = (w - 12) / 2;
  const int bh = 32;

  const bool yesActive = s.popupYesSelected;
  const bool noActive  = !s.popupYesSelected;

  drawPill(x + 0,       by, bw, bh, "No",  noActive, true);
  drawPill(x + bw + 12, by, bw, bh, "Yes", yesActive, true);
}

void DisplayUI::render(const UiState& s) {
  _gfx = _lcd;

  _gfx->fillScreen(C_BG(_lcd));
  drawTopBar(s);

  switch (s.screen) {
    case UiScreen::MAIN:
      drawMain(s);
      drawFooterMain(s);
      break;
    case UiScreen::SETTINGS:
      drawSettings(s);
      break;
    case UiScreen::SET_POS:
      drawSetPos(s);
      break;
    case UiScreen::WIZARD:
      drawWizard(s);
      break;
    case UiScreen::RUN:
      drawRun(s);
      break;
    case UiScreen::POPUP:
      drawRun(s);
      drawPopup(s);
      break;
    default:
      break;
  }
}