#pragma once

#include <M5Unified.h>

namespace Ui { namespace Icons {

// All icons are 16x16, monochrome, encoded as 16 strings of 16 chars where
// '#' marks a lit pixel and any other character is transparent.
constexpr int W = 16;
constexpr int H = 16;
using Icon = const char* const*;

extern const char* const HomeIcon[H];
extern const char* const DxIcon[H];
extern const char* const PropIcon[H];
extern const char* const AlertsIcon[H];
extern const char* const SettingsIcon[H];
extern const char* const RefreshIcon[H];

// Draw at (x, y) (top-left), pixels in `color`. Transparent pixels are not
// touched, so this composites cleanly over an existing background.
void draw(int x, int y, Icon icon, uint16_t color);

} }
