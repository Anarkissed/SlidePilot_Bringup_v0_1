#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

// Marker state model (matches the combinations you listed)
struct MarkerState {
  bool set;      // incomplete vs complete
  bool easeIn;
  bool easeOut;
  bool pause;

  constexpr MarkerState() : set(false), easeIn(false), easeOut(false), pause(false) {}
  constexpr MarkerState(bool s, bool in, bool out, bool p)
      : set(s), easeIn(in), easeOut(out), pause(p) {}
};

enum class MarkerIconKind : uint8_t {
  INCOMPLETE = 0,
  SET,
  EASE_IN,
  EASE_OUT,
  EASE_BOTH,
  PAUSE,
  EASE_IN_PAUSE,
  PAUSE_EASE_OUT,
  EASE_IN_PAUSE_EASE_OUT
};

// Map MarkerState -> icon kind
static inline MarkerIconKind markerIconFromState(const MarkerState& m) {
  if (!m.set) return MarkerIconKind::INCOMPLETE;

  const bool in  = m.easeIn;
  const bool out = m.easeOut;
  const bool p   = m.pause;

  if (!p) {
    if (!in && !out) return MarkerIconKind::SET;
    if ( in && !out) return MarkerIconKind::EASE_IN;
    if (!in &&  out) return MarkerIconKind::EASE_OUT;
    return MarkerIconKind::EASE_BOTH;
  } else {
    if (!in && !out) return MarkerIconKind::PAUSE;
    if ( in && !out) return MarkerIconKind::EASE_IN_PAUSE;
    if (!in &&  out) return MarkerIconKind::PAUSE_EASE_OUT;
    return MarkerIconKind::EASE_IN_PAUSE_EASE_OUT;
  }
}

// Draw a marker icon centered at (cx, cy). r is radius of outer circle.
void drawMarkerIcon(lgfx::LGFX_Device* g,
                    int cx, int cy, int r,
                    MarkerIconKind kind,
                    bool selected,
                    uint32_t colBg,
                    uint32_t colFg,
                    uint32_t colAccent,
                    uint32_t colDim);

// Draw a simple camera icon centered at (cx, cy)
void drawCameraIcon(lgfx::LGFX_Device* g,
                    int cx, int cy,
                    int w, int h,
                    uint32_t colFg,
                    uint32_t colFill);

// Draw a down arrow between camera and marker
void drawDownArrow(lgfx::LGFX_Device* g,
                   int cx, int topY, int bottomY,
                   uint32_t col);