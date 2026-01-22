#include "Display.h"      // for MarkerState
#include "MarkerIcons.h"

MarkerIconKind markerIconFromState(const MarkerState& m) {
  // Minimal mapping used by the current UI:
  // - EMPTY when a marker is not set
  // - DOT when it is set
  if (!m.set) return MarkerIconKind::EMPTY;
  return MarkerIconKind::DOT;
}