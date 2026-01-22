#include "Display.h"

#include <Arduino.h>

namespace {

// -----------------------------------------------------------------------------
// Theme
// -----------------------------------------------------------------------------

static constexpr uint16_t C_BG     = 0x0000; // black
static constexpr uint16_t C_SURF   = 0x18E3; // dark gray-blue
static constexpr uint16_t C_PILL   = 0x2104; // slightly lighter
static constexpr uint16_t C_TEXT   = 0xFFFF; // white
static constexpr uint16_t C_DIM    = 0x8410; // mid gray
static constexpr uint16_t C_ACCENT = 0x3D7F; // a brighter cyan-ish

static int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// -----------------------------------------------------------------------------
// Layout metrics
// -----------------------------------------------------------------------------
struct Metrics {
  int W = 0;
  int H = 0;
  int pad = 8;

  // Top pills
  int headerX = 0;
  int headerY = 0;
  int headerW = 0;
  int headerH = 28;
  int headerR = 14;

  int topRightX = 0;
  int topRightY = 0;
  int topRightW = 86;
  int topRightH = 28;
  int topRightR = 14;

  // Content card
  int contentX = 0;
  int contentY = 0;
  int contentW = 0;
  int contentH = 0;
  int contentR = 22;

  // Bottom pills
  int bottomY = 0;
  int pillH = 26;
  int pillW = 78;
  int pillR = 13;
};

static Metrics calcMetrics(lgfx::LGFX_Device* lcd) {
  Metrics m;
  m.W = lcd->width();
  m.H = lcd->height();
  m.pad = 8;

  m.headerH = 28;
  m.headerR = m.headerH / 2;
  m.headerW = m.W / 2;   // half screen width
  m.headerX = m.pad;
  m.headerY = m.pad;

  m.topRightW = 86;
  m.topRightH = 28;
  m.topRightR = m.topRightH / 2;
  m.topRightX = m.W - m.pad - m.topRightW;
  m.topRightY = m.pad;

  m.bottomY = m.H - m.pad - m.pillH;

  // content card sits between top pills row and bottom pills row
  const int contentTop = m.headerY + m.headerH + 8;
  const int contentBottom = m.bottomY - 8;
  m.contentX = m.pad;
  m.contentY = contentTop;
  m.contentW = m.W - (m.pad * 2);
  m.contentH = contentBottom - contentTop;
  m.contentR = 22;

  return m;
}

// -----------------------------------------------------------------------------
// Drawing helpers
// -----------------------------------------------------------------------------

static void safeString(lgfx::LovyanGFX* g, const char* txt, int x, int y) {
  g->drawString(txt ? txt : "", x, y);
}

static void pill(lgfx::LovyanGFX* g,
                 int x, int y, int w, int h,
                 const char* text,
                 bool selected,
                 bool big,
                 bool disabled = false) {
  const int r = h / 2;
  g->fillRoundRect(x, y, w, h, r, C_PILL);
  if (selected) {
    g->drawRoundRect(x - 1, y - 1, w + 2, h + 2, r + 1, C_ACCENT);
  }

  g->setTextDatum(middle_center);
  g->setTextSize(big ? 2 : 1);
  g->setTextColor(disabled ? C_DIM : C_TEXT);
  safeString(g, text, x + w / 2, y + h / 2);
}

// Camera icon (outline primitives)
static void drawCameraIcon(lgfx::LovyanGFX* g, int cx, int cy, int sz, uint16_t col) {
  const int w = sz;
  const int h = (sz * 3) / 5;
  const int x = cx - w / 2;
  const int y = cy - h / 2;

  const int r = 6;

  // main body outline
  g->drawRoundRect(x, y, w, h, r, col);
  g->drawRoundRect(x + 1, y + 1, w - 2, h - 2, r, col);

  // top bump
  const int bumpW = (w * 2) / 5;
  const int bumpH = (h * 2) / 5;
  const int bumpX = x + (w - bumpW) / 2;
  const int bumpY = y - bumpH / 2;
  g->drawRoundRect(bumpX, bumpY, bumpW, bumpH, 4, col);

  // lens circle
  const int lr = h / 3;
  g->drawCircle(cx, cy, lr, col);
  g->drawCircle(cx, cy, lr - 1, col);

  // small dot
  g->fillCircle(x + (w * 1) / 6, y + (h * 1) / 3, 2, col);
}

// Down triangle (pointing down)
static void drawDownTriangle(lgfx::LovyanGFX* g, int cx, int cy, int w, int h, uint16_t col) {
  g->fillTriangle(cx, cy + h / 2, cx - w / 2, cy - h / 2, cx + w / 2, cy - h / 2, col);
}

// Mode icons (bottom center) — NO mode name text here
static void drawModeIcon(lgfx::LovyanGFX* g, SlideMode mode, int cx, int cy, int sz, uint16_t col) {
  if (mode == SlideMode::Single) {
    // →
    const int w = sz;
    const int h = sz / 2;
    g->drawLine(cx - w / 2, cy, cx + w / 2 - 6, cy, col);
    g->fillTriangle(cx + w / 2 - 6, cy,
                    cx + w / 2 - 14, cy - h / 2,
                    cx + w / 2 - 14, cy + h / 2, col);
  } else if (mode == SlideMode::Bounce) {
    // ↔
    const int w = sz;
    const int h = sz / 2;
    g->drawLine(cx - w / 2 + 6, cy, cx + w / 2 - 6, cy, col);
    g->fillTriangle(cx - w / 2 + 6, cy,
                    cx - w / 2 + 14, cy - h / 2,
                    cx - w / 2 + 14, cy + h / 2, col);
    g->fillTriangle(cx + w / 2 - 6, cy,
                    cx + w / 2 - 14, cy - h / 2,
                    cx + w / 2 - 14, cy + h / 2, col);
  } else {
    // stacked frames
    const int s = sz / 2;
    g->drawRect(cx - s,     cy - s,     s * 2, s * 2, col);
    g->drawRect(cx - s - 4, cy - s - 4, s * 2, s * 2, col);
    g->drawRect(cx - s - 8, cy - s - 8, s * 2, s * 2, col);
  }
}

static const char* modeTitle(SlideMode m) {
  switch (m) {
    case SlideMode::Single:    return "SINGLE";
    case SlideMode::Bounce:    return "BOUNCE";
    case SlideMode::Timelapse: return "TIMELAPSE";
  }
  return "SINGLE";
}

static void markerPositions(const Metrics& m, int outX[UiState::kMaxMarkers]) {
  const int left  = m.contentX + 44;
  const int right = m.contentX + m.contentW - 44;
  const int n = UiState::kMaxMarkers;
  for (int i = 0; i < n; ++i) {
    outX[i] = left + (i * (right - left)) / (n - 1);
  }
}

} // namespace

// ----------------------------------------------------------------------------
// DisplayUI implementation
// ----------------------------------------------------------------------------

bool DisplayUI::begin(lgfx::LGFX_Device* dev) {
  _lcd = dev;
  if (!_lcd) return false;

  _W = _lcd->width();
  _H = _lcd->height();

  // ✅ CRITICAL FIX:
  // Disable full-frame sprites for now — this is what’s causing the boot-loop.
  _useSprite = false;

  return true;
}

void DisplayUI::render(const UiState& s) {
  if (!_lcd) return;

  // ✅ Direct draw (no sprite = no crash)
  lgfx::LovyanGFX* g = _lcd;

  const Metrics m = calcMetrics(_lcd);

  // Base clear
  g->fillScreen(C_BG);

  // Header pill (top-left) always mode title
  pill(g, m.headerX, m.headerY, m.headerW, m.headerH, modeTitle(s.mode), false, true);

  // Top-right settings pill only Main + Settings
  const bool showSettingsPill = (s.screen == UiScreen::Main || s.screen == UiScreen::Settings);
  if (showSettingsPill) {
    const bool sel = (s.screen == UiScreen::Main && s.mainFocus == MainFocus::Settings);
    pill(g, m.topRightX, m.topRightY, m.topRightW, m.topRightH, "Settings", sel, false);
  }

  // Content container
  g->fillRoundRect(m.contentX, m.contentY, m.contentW, m.contentH, m.contentR, C_SURF);

  // ---------------------------------------------------------------------------
  // MAIN
  // ---------------------------------------------------------------------------
  if (s.screen == UiScreen::Main) {
    const int innerX = m.contentX;
    const int innerY = m.contentY;
    const int innerW = m.contentW;
    const int innerH = m.contentH;

    // pad -> camera -> pad -> triangle -> pad -> timeline -> pad
    const int topPad = 14;
    const int bottomPad = 14;
    const int cameraSz = 42;
    const int triW = 18;
    const int triH = 14;
    const int tlH = 40;

    const int total = topPad + cameraSz + topPad + triH + topPad + tlH + bottomPad;
    int startY = innerY + (innerH - total) / 2;
    if (startY < innerY + 10) startY = innerY + 10;

    const int camCX = innerX + 30;
    const int camCY = startY + topPad + cameraSz / 2;

    const bool focusIsMarker = (s.mainFocus >= MainFocus::Marker1 && s.mainFocus <= MainFocus::Marker6);
    const int focusMarkerIdx = focusIsMarker
      ? (static_cast<int>(s.mainFocus) - static_cast<int>(MainFocus::Marker1))
      : -1;

    const bool showTriangleHighlight = focusIsMarker;

    drawCameraIcon(g, camCX, camCY, cameraSz, showTriangleHighlight ? C_ACCENT : C_TEXT);

    const int triCX = camCX;
    const int triCY = camCY + (cameraSz / 2) + topPad + triH / 2;
    drawDownTriangle(g, triCX, triCY, triW, triH, showTriangleHighlight ? C_ACCENT : C_TEXT);

    const int tlY = triCY + triH / 2 + topPad;
    const int baseY = tlY + (tlH * 2) / 3;

    int posX[UiState::kMaxMarkers];
    markerPositions(m, posX);

    g->drawLine(posX[0], baseY, posX[UiState::kMaxMarkers - 1], baseY, C_DIM);

    if (showTriangleHighlight) {
      const int idx = clampi(focusMarkerIdx, 0, UiState::kMaxMarkers - 1);
      const int ax = posX[idx];
      const int ay = baseY - 24;
      g->fillTriangle(ax, ay, ax - 8, ay + 10, ax + 8, ay + 10, C_ACCENT);
    }

    for (int i = 0; i < UiState::kMaxMarkers; ++i) {
      const bool activeSlot = (i < s.markerCount);
      const bool set = activeSlot ? s.markers[i].set : false;
      const bool focused = (focusIsMarker && i == focusMarkerIdx);
      const int r = 10;

      g->fillCircle(posX[i], baseY, r, C_SURF);
      g->drawCircle(posX[i], baseY, r, focused ? C_ACCENT : (activeSlot ? C_TEXT : C_DIM));

      if (set) {
        g->fillCircle(posX[i], baseY, 4, focused ? C_ACCENT : C_TEXT);
      }
    }

    // Bottom pills
    const int nextX = m.contentX + 14;
    const int nextY = m.bottomY;
    const int nextW = 78;
    const int modeW = 78;
    const int modeX = m.W - m.pad - modeW;

    const bool nextSel = (s.mainFocus == MainFocus::Next);
    const bool modeSel = (s.mainFocus == MainFocus::Mode);
    pill(g, nextX, nextY, nextW, m.pillH, "Next", nextSel, false);
    pill(g, modeX, nextY, modeW, m.pillH, "Mode", modeSel, false);

    // Mode icon between pills
    const int iconCX = (nextX + nextW + modeX) / 2;
    const int iconCY = nextY + m.pillH / 2;
    drawModeIcon(g, s.mode, iconCX, iconCY, 20, C_TEXT);

    return;
  }

  // ---------------------------------------------------------------------------
  // Fallback screens (so we always show something)
  // ---------------------------------------------------------------------------
  g->setTextDatum(middle_center);
  g->setTextSize(2);
  g->setTextColor(C_TEXT);
  safeString(g, "RUNNING", m.W / 2, m.H / 2);
}