#include "Display.h"

#include <algorithm>

#include "MarkerIcons.h"

namespace {

// --- Theme colors (RGB565) ---
static constexpr uint16_t C_BG     = 0x0000; // black
static constexpr uint16_t C_SURF   = 0x18E3; // dark gray-blue
static constexpr uint16_t C_PILL   = 0x2104; // slightly lighter
static constexpr uint16_t C_TEXT   = 0xFFFF; // white
static constexpr uint16_t C_DIM    = 0x8410; // mid gray
static constexpr uint16_t C_ACCENT = 0x07FF; // cyan

struct Metrics {
  int16_t W = 0;
  int16_t H = 0;
  int16_t pad = 8;

  // Header
  int16_t headerX = 0;
  int16_t headerY = 0;
  int16_t headerW = 0;
  int16_t headerH = 0;
  int16_t headerR = 0;

  // Content
  int16_t contentX = 0;
  int16_t contentY = 0;
  int16_t contentW = 0;
  int16_t contentH = 0;
  int16_t contentR = 0;

  // Bottom pills
  int16_t bottomY = 0;
  int16_t pillH = 26;
  int16_t pillW = 92;
  int16_t pillR = 13;
  int16_t gap = 10;
};

inline int16_t clampi(int16_t v, int16_t lo, int16_t hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

static Metrics computeMetrics(lgfx::LGFX_Device* lcd) {
  Metrics m;
  m.W = static_cast<int16_t>(lcd->width());
  m.H = static_cast<int16_t>(lcd->height());
  m.pad = 8;

  m.headerX = m.pad;
  m.headerY = m.pad;
  m.headerW = std::max<int16_t>(m.W / 2, 140); // note: header pill should be ~half screen, never truncate titles
  m.headerH = 26;
  m.headerR = m.headerH / 2;

  // Bottom area = pad + pills + pad (to match top padding reference)
  m.bottomY = m.H - (m.pad + m.pillH);

  m.contentX = m.pad;
  m.contentY = m.headerY + m.headerH + m.pad;
  m.contentW = m.W - 2 * m.pad;
  m.contentH = m.bottomY - m.pad - m.contentY; // ensures bottom padding equals top pad
  m.contentH = std::max<int16_t>(m.contentH, 30);
  m.contentR = 18;

  // Adjust pill size if needed for narrow screens
  int16_t total = (3 * m.pillW) + (2 * m.gap);
  if (total > (m.W - 2 * m.pad)) {
    int16_t avail = (m.W - 2 * m.pad) - (2 * m.gap);
    m.pillW = clampi(avail / 3, 60, 110);
  }

  return m;
}

static void drawPill(lgfx::LGFX_Sprite& spr, int x, int y, int w, int h, uint16_t fill,
                     const char* text, bool selected, bool dimText, int textSize = 1) {
  int r = h / 2;
  spr.fillRoundRect(x, y, w, h, r, fill);
  if (selected) {
    spr.drawRoundRect(x - 1, y - 1, w + 2, h + 2, r + 1, C_ACCENT);
  }

  spr.setTextDatum(middle_center);
  spr.setTextSize(textSize);
  spr.setTextColor(dimText ? C_DIM : C_TEXT);
  spr.drawString(text, x + w / 2, y + h / 2);
}

static void drawHeader(lgfx::LGFX_Sprite& spr, const Metrics& m, const UiState& s) {
  // Header pill always visible with latest header; make the text a little larger.
  spr.fillRoundRect(m.headerX, m.headerY, m.headerW, m.headerH, m.headerR, C_PILL);
  spr.setTextDatum(middle_center);
  spr.setTextSize(2);
  spr.setTextColor(C_TEXT);
  spr.drawString(s.header, m.headerX + m.headerW / 2, m.headerY + m.headerH / 2);
}

static void drawContentFrame(lgfx::LGFX_Sprite& spr, const Metrics& m) {
  spr.fillRoundRect(m.contentX, m.contentY, m.contentW, m.contentH, m.contentR, C_SURF);
}

static void drawMainScreen(lgfx::LGFX_Sprite& spr, const Metrics& m, const UiState& s) {
  // Content title
  spr.setTextDatum(top_left);
  spr.setTextSize(2);
  spr.setTextColor(C_TEXT);
  spr.drawString("Main", m.contentX + 14, m.contentY + 12);

  // Marker row + timeline (simple bring-up rendering)
  const int rowY = m.contentY + 52;
  const int centerY = rowY + 38;
  const int iconSz = 34;
  const int arrowH = 14;

  const int leftX = m.contentX + 20;
  const int rightX = m.contentX + m.contentW - 20;

  const int positions[3] = {
      leftX,
      m.contentX + m.contentW / 2,
      rightX,
  };

  // Timeline line behind icons
  spr.drawLine(positions[0], centerY + iconSz / 2, positions[2], centerY + iconSz / 2, C_DIM);

  // Arrow is ABOVE marker (per your note)
  if (s.mainFocus >= MainFocus::MARKER1 && s.mainFocus <= MainFocus::MARKER3) {
    int idx = static_cast<int>(s.mainFocus) - static_cast<int>(MainFocus::MARKER1);
    idx = clampi(idx, 0, 2);
    int ax = positions[idx];
    int ay = centerY - arrowH - 6;
    spr.fillTriangle(ax, ay, ax - 8, ay + arrowH, ax + 8, ay + arrowH, C_ACCENT);
  }

  // Marker icons
  for (int i = 0; i < 3; ++i) {
    bool focused = (s.mainFocus == static_cast<MainFocus>(static_cast<int>(MainFocus::MARKER1) + i));
    const MarkerState& ms = s.markers[i];
    MarkerIconKind kind = MarkerIconKind::EMPTY;
    if (ms.set) {
      kind = MarkerIconKind::CHECK;
    }
    const IconSpec spec = getMarkerIcon(kind);
    drawIcon(spr, spec, positions[i] - iconSz / 2, centerY, iconSz, focused ? C_ACCENT : C_TEXT);
  }

  // Hint text (kept within content area)
  spr.setTextDatum(bottom_left);
  spr.setTextSize(1);
  spr.setTextColor(C_DIM);
  spr.drawString("Back saves & exits", m.contentX + 14, m.contentY + m.contentH - 14);
}

static void drawSettingsScreen(lgfx::LGFX_Sprite& spr, const Metrics& m, const UiState& s) {
  spr.setTextDatum(top_left);
  spr.setTextSize(2);
  spr.setTextColor(C_TEXT);
  spr.drawString("Settings", m.contentX + 14, m.contentY + 12);

  spr.setTextSize(1);
  spr.setTextColor(C_DIM);
  spr.drawString("(bring-up)", m.contentX + 14, m.contentY + 38);
}

static void drawRunningScreen(lgfx::LGFX_Sprite& spr, const Metrics& m, const UiState& s) {
  spr.setTextDatum(top_left);
  spr.setTextSize(2);
  spr.setTextColor(C_TEXT);
  spr.drawString("Running", m.contentX + 14, m.contentY + 12);

  spr.setTextSize(1);
  spr.setTextColor(C_DIM);
  spr.drawString("Press Cancel to stop", m.contentX + 14, m.contentY + 42);

  if (s.popup == PopupKind::CANCEL_CONFIRM) {
    // Simple modal
    const int pw = std::min<int>(240, m.contentW - 20);
    const int ph = 110;
    const int px = m.contentX + (m.contentW - pw) / 2;
    const int py = m.contentY + (m.contentH - ph) / 2;
    spr.fillRoundRect(px, py, pw, ph, 18, C_PILL);
    spr.drawRoundRect(px, py, pw, ph, 18, C_ACCENT);

    spr.setTextDatum(top_center);
    spr.setTextSize(2);
    spr.setTextColor(C_TEXT);
    spr.drawString("Cancel move?", px + pw / 2, py + 10);

    // Default highlight is NO (per your note)
    bool yesSelected = (s.cancelConfirmSelection == CancelConfirmSelection::YES);
    int by = py + ph - 40;
    int bw = (pw - 30) / 2;
    drawPill(spr, px + 10, by, bw, 26, C_SURF, "No", !yesSelected, false, 1);
    drawPill(spr, px + 20 + bw, by, bw, 26, C_SURF, "Yes", yesSelected, false, 1);
  }
}

static void drawBottomPills(lgfx::LGFX_Sprite& spr, const Metrics& m, const UiState& s) {
  int totalW = (3 * m.pillW) + (2 * m.gap);
  int x0 = m.pad + (m.W - 2 * m.pad - totalW) / 2;
  int y = m.bottomY;

  bool backSel = (s.mainFocus == MainFocus::BACK);
  bool setSel = (s.mainFocus == MainFocus::SETTINGS);
  bool nextSel = (s.mainFocus == MainFocus::NEXT);

  drawPill(spr, x0, y, m.pillW, m.pillH, C_PILL, "Back", backSel, false, 1);
  drawPill(spr, x0 + m.pillW + m.gap, y, m.pillW, m.pillH, C_PILL, "Settings", setSel, false, 1);
  drawPill(spr, x0 + 2 * (m.pillW + m.gap), y, m.pillW, m.pillH, C_PILL, "Next", nextSel, false, 1);
}

} // namespace

void DisplayUI::begin(lgfx::LGFX_Device* dev) {
  _lcd = dev;
  if (!_lcd) {
    return;
  }

  // Create sprite after the panel is initialized.
  _spr.setColorDepth(16);
  _spr.setFont(&fonts::Font2);
  _spr.setPsram(true);
  _spr.createSprite(_lcd->width(), _lcd->height());
}

void DisplayUI::draw(const UiState& state) {
  if (!_lcd) {
    return;
  }

  // Recreate sprite if rotation/size changed.
  if (_spr.width() != _lcd->width() || _spr.height() != _lcd->height()) {
    _spr.deleteSprite();
    _spr.setColorDepth(16);
    _spr.setPsram(true);
    _spr.createSprite(_lcd->width(), _lcd->height());
  }

  const Metrics m = computeMetrics(_lcd);

  _spr.fillScreen(C_BG);

  drawHeader(_spr, m, state);
  drawContentFrame(_spr, m);

  switch (state.screen) {
    case UiScreen::MAIN:
      drawMainScreen(_spr, m, state);
      break;
    case UiScreen::SETTINGS:
      drawSettingsScreen(_spr, m, state);
      break;
    case UiScreen::RUNNING:
      drawRunningScreen(_spr, m, state);
      break;
    default:
      // fallback
      _spr.setTextDatum(middle_center);
      _spr.setTextSize(2);
      _spr.setTextColor(C_TEXT);
      _spr.drawString("(unimplemented)", m.W / 2, m.H / 2);
      break;
  }

  drawBottomPills(_spr, m, state);

  _spr.pushSprite(0, 0);
}
