#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

// IMPORTANT:
// These are templates so they work with BOTH LGFX_Device* and LGFX_Sprite*.
// That fixes your error: cannot convert 'LGFX_Sprite*' to 'LGFX_Device*'.

enum class MarkerIconKind : uint8_t {
  EMPTY = 0,
  DOT,
  EASE_IN,
  EASE_OUT,
  EASE_IN_OUT,
  PAUSE
};

template <typename GFX>
inline void drawDownTriangle(GFX* g, int cx, int cy, int h, uint32_t color) {
  // Simple filled triangle pointing down
  const int half = h / 2;
  g->fillTriangle(cx - half, cy - half, cx + half, cy - half, cx, cy + half, color);
}

template <typename GFX>
inline void drawCameraIcon(GFX* g, int x, int y, int w, int h, uint32_t stroke, uint32_t /*bg*/) {
  // Camera icon (stroke-only style, based on your SVG proportions)
  // Body
  g->drawRoundRect(x, y + (h/6), w, (h*5)/6, 8, stroke);
  // Top hump
  g->drawRoundRect(x + (w/8), y, (w*3)/8, (h/3), 6, stroke);
  // Lens
  const int cx = x + (w*5)/8;
  const int cy = y + (h*3)/5;
  const int r  = min(w, h) / 4;
  g->drawCircle(cx, cy, r, stroke);
  g->drawCircle(cx, cy, r-1, stroke);
  // Small dot
  g->fillCircle(x + (w/6), y + (h/3), 3, stroke);
}

template <typename GFX>
inline void drawMarkerIcon(
  GFX* g,
  int cx, int cy,
  int size,
  MarkerIconKind kind,
  bool selected,
  uint32_t stroke,
  uint32_t bg,
  uint32_t accent,
  uint32_t dim
) {
  const int r = size / 2;

  // subtle background chip for marker
  g->fillCircle(cx, cy, r, bg);
  g->drawCircle(cx, cy, r, selected ? accent : dim);

  const uint32_t ink = selected ? accent : stroke;

  switch (kind) {
    case MarkerIconKind::EMPTY:
      break;

    case MarkerIconKind::DOT:
      g->fillCircle(cx, cy, max(2, r/4), ink);
      break;

    case MarkerIconKind::EASE_IN: {
      // arrow pointing right
      g->drawLine(cx - r/3, cy, cx + r/3, cy, ink);
      g->drawLine(cx + r/3, cy, cx + r/12, cy - r/6, ink);
      g->drawLine(cx + r/3, cy, cx + r/12, cy + r/6, ink);
    } break;

    case MarkerIconKind::EASE_OUT: {
      // arrow pointing left
      g->drawLine(cx - r/3, cy, cx + r/3, cy, ink);
      g->drawLine(cx - r/3, cy, cx - r/12, cy - r/6, ink);
      g->drawLine(cx - r/3, cy, cx - r/12, cy + r/6, ink);
    } break;

    case MarkerIconKind::EASE_IN_OUT: {
      // double arrows
      g->drawLine(cx - r/3, cy, cx + r/3, cy, ink);
      g->drawLine(cx + r/3, cy, cx + r/12, cy - r/6, ink);
      g->drawLine(cx + r/3, cy, cx + r/12, cy + r/6, ink);

      g->drawLine(cx - r/3, cy, cx - r/12, cy - r/6, ink);
      g->drawLine(cx - r/3, cy, cx - r/12, cy + r/6, ink);
    } break;

    case MarkerIconKind::PAUSE: {
      // pause bars
      g->fillRect(cx - r/4, cy - r/3, r/6, (2*r)/3, ink);
      g->fillRect(cx + r/12, cy - r/3, r/6, (2*r)/3, ink);
    } break;
  }
}

template <typename GFX>
inline void drawModeIcon(GFX* g, int cx, int cy, int size, uint8_t modeIndex, uint32_t color) {
  // modeIndex: 0=Single, 1=Bounce, 2=Timelapse
  const int r = size / 2;

  if (modeIndex == 2) {
    // stacked frames (timelapse)
    g->drawRect(cx - r, cy - r, size, size, color);
    g->drawRect(cx - r - 6, cy - r + 6, size, size, color);
    g->drawRect(cx - r - 12, cy - r + 12, size, size, color);
    return;
  }

  // Single/Bounce arrows (between bottom pills)
  // Single: one arrow → ; Bounce: double arrow ⇄
  // We'll draw as simple chevrons.
  auto chevronRight = [&](int ox) {
    g->drawLine(cx - r/2 + ox, cy - r/3, cx + r/2 + ox, cy, color);
    g->drawLine(cx - r/2 + ox, cy + r/3, cx + r/2 + ox, cy, color);
  };
  auto chevronLeft = [&](int ox) {
    g->drawLine(cx + r/2 + ox, cy - r/3, cx - r/2 + ox, cy, color);
    g->drawLine(cx + r/2 + ox, cy + r/3, cx - r/2 + ox, cy, color);
  };

  if (modeIndex == 0) {
    chevronRight(0);
  } else {
    chevronLeft(-2);
    chevronRight(+2);
  }
}