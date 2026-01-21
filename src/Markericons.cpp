#include "MarkerIcons.h"

static void drawCheck(lgfx::LGFX_Device* g, int cx, int cy, int s, uint32_t col) {
  // simple check mark (two thick lines)
  g->drawLine(cx - s, cy,     cx - s/3, cy + s, col);
  g->drawLine(cx - s + 1, cy, cx - s/3 + 1, cy + s, col);

  g->drawLine(cx - s/3, cy + s, cx + s, cy - s, col);
  g->drawLine(cx - s/3, cy + s - 1, cx + s, cy - s - 1, col);
}

static void drawPauseBars(lgfx::LGFX_Device* g, int cx, int cy, int w, int h, uint32_t col) {
  const int barW = max(2, w / 5);
  const int gap  = max(2, barW);
  const int x1 = cx - gap/2 - barW;
  const int x2 = cx + gap/2;
  const int y  = cy - h/2;
  g->fillRect(x1, y, barW, h, col);
  g->fillRect(x2, y, barW, h, col);
}

static void drawEaseArrow(lgfx::LGFX_Device* g, int cx, int cy, int len, bool left, uint32_t col) {
  // horizontal arrow with triangle head
  const int y = cy;
  const int x1 = cx - len/2;
  const int x2 = cx + len/2;
  g->drawLine(x1, y, x2, y, col);

  if (left) {
    g->fillTriangle(x1, y, x1 + 6, y - 5, x1 + 6, y + 5, col);
  } else {
    g->fillTriangle(x2, y, x2 - 6, y - 5, x2 - 6, y + 5, col);
  }
}

static void drawDoubleEase(lgfx::LGFX_Device* g, int cx, int cy, int len, uint32_t col) {
  // two arrows, left and right
  drawEaseArrow(g, cx, cy - 5, len, true,  col);
  drawEaseArrow(g, cx, cy + 5, len, false, col);
}

void drawMarkerIcon(lgfx::LGFX_Device* g,
                    int cx, int cy, int r,
                    MarkerIconKind kind,
                    bool selected,
                    uint32_t colBg,
                    uint32_t colFg,
                    uint32_t colAccent,
                    uint32_t colDim)
{
  const uint32_t ring = selected ? colAccent : colDim;

  g->fillCircle(cx, cy, r, colBg);
  g->drawCircle(cx, cy, r, ring);
  g->drawCircle(cx, cy, r - 1, ring);

  const int s = max(6, r - 5);

  switch (kind) {
    case MarkerIconKind::INCOMPLETE: {
      g->drawCircle(cx, cy, r - 5, colDim);
      g->fillCircle(cx, cy, 2, colDim);
    } break;

    case MarkerIconKind::SET: {
      drawCheck(g, cx, cy, s/2, colFg);
    } break;

    case MarkerIconKind::EASE_IN: {
      drawEaseArrow(g, cx, cy, s + 6, true, colFg);
    } break;

    case MarkerIconKind::EASE_OUT: {
      drawEaseArrow(g, cx, cy, s + 6, false, colFg);
    } break;

    case MarkerIconKind::EASE_BOTH: {
      drawDoubleEase(g, cx, cy, s + 4, colFg);
    } break;

    case MarkerIconKind::PAUSE: {
      drawPauseBars(g, cx, cy, s, s + 4, colFg);
    } break;

    case MarkerIconKind::EASE_IN_PAUSE: {
      drawEaseArrow(g, cx - 6, cy, s, true, colFg);
      drawPauseBars(g, cx + 6, cy, s, s + 2, colFg);
    } break;

    case MarkerIconKind::PAUSE_EASE_OUT: {
      drawPauseBars(g, cx - 6, cy, s, s + 2, colFg);
      drawEaseArrow(g, cx + 6, cy, s, false, colFg);
    } break;

    case MarkerIconKind::EASE_IN_PAUSE_EASE_OUT: {
      drawEaseArrow(g, cx - 10, cy, s - 2, true, colFg);
      drawPauseBars(g, cx, cy, s - 2, s + 2, colFg);
      drawEaseArrow(g, cx + 10, cy, s - 2, false, colFg);
    } break;
  }
}

void drawCameraIcon(lgfx::LGFX_Device* g,
                    int cx, int cy,
                    int w, int h,
                    uint32_t colFg,
                    uint32_t colFill)
{
  const int x = cx - w/2;
  const int y = cy - h/2;

  const int r = max(4, min(w, h)/5);
  g->fillRoundRect(x, y, w, h, r, colFill);
  g->drawRoundRect(x, y, w, h, r, colFg);

  // top bump (viewfinder)
  const int bumpW = max(8, w/3);
  const int bumpH = max(4, h/4);
  g->fillRoundRect(cx - bumpW/2, y - bumpH + 2, bumpW, bumpH, bumpH/2, colFill);
  g->drawRoundRect(cx - bumpW/2, y - bumpH + 2, bumpW, bumpH, bumpH/2, colFg);

  // lens
  const int lr = max(4, min(w, h)/4);
  g->fillCircle(cx, cy, lr, colFg);
  g->fillCircle(cx, cy, lr - 2, colFill);

  // small LED
  g->fillCircle(x + w - 8, y + 8, 2, colFg);
}

void drawDownArrow(lgfx::LGFX_Device* g,
                   int cx, int topY, int bottomY,
                   uint32_t col)
{
  g->drawLine(cx, topY, cx, bottomY - 6, col);
  g->fillTriangle(cx - 6, bottomY - 6, cx + 6, bottomY - 6, cx, bottomY, col);
}