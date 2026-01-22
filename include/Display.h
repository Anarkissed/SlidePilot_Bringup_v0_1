#pragma once

#include <LovyanGFX.hpp>

#include "UiState.h"

// ----------------------------------------------------------------------------
// DisplayUI
//
// A single renderer that draws the TFT UI using a full-frame sprite (double
// buffering) when possible to eliminate tearing / "wave" refresh artifacts.
//
// If sprite allocation fails for any reason, it falls back to direct LCD draw.
// ----------------------------------------------------------------------------

class DisplayUI {
public:
  bool begin(lgfx::LGFX_Device* dev);
  void render(const UiState& s);

private:
  lgfx::LGFX_Device* _lcd = nullptr;
  lgfx::LGFX_Sprite  _spr;
  bool _useSprite = false;

  int _W = 0;
  int _H = 0;

  // Font handling
  bool _hasCustomFont = false;
};