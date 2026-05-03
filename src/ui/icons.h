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
extern const char* const BeaconIcon[H];     // radio tower with signal lobes
extern const char* const PotaIcon[H];       // pine tree (parks on the air)
extern const char* const BearingIcon[H];    // compass rose / arrow
extern const char* const NoaaIcon[H];       // warning triangle
extern const char* const ContestIcon[H];    // trophy
extern const char* const GraylineIcon[H];   // globe with terminator
extern const char* const SatelliteIcon[H];  // satellite body with solar panels
extern const char* const SotaIcon[H];       // mountain peak (summits on the air)

// Draw at (x, y) (top-left), pixels in `color`. Transparent pixels are not
// touched, so this composites cleanly over an existing background.
void draw(int x, int y, Icon icon, uint16_t color);

} }
