#pragma once

#include <M5Unified.h>
#include "ui.h"

namespace Ui {

// Layout constants for the touch UI.
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
constexpr int TAB_H    = 32;
constexpr int CONTENT_Y = TAB_H;
constexpr int CONTENT_H = SCREEN_H - TAB_H;

constexpr uint16_t COL_BG       = 0x0000;     // black
constexpr uint16_t COL_FG       = 0xFFFF;     // white
constexpr uint16_t COL_DIM      = 0x8410;     // grey
constexpr uint16_t COL_ACCENT   = 0x07FF;     // cyan
constexpr uint16_t COL_TAB_BG   = 0x2104;     // dark grey
constexpr uint16_t COL_TAB_SEL  = 0x041F;     // navy/blue
constexpr uint16_t COL_OK       = 0x07E0;     // green
constexpr uint16_t COL_WARN     = 0xFD20;     // orange
constexpr uint16_t COL_BAD      = 0xF800;     // red
constexpr uint16_t COL_BANNER   = 0xFD20;

// Per-screen draw / touch handlers.
namespace ScreenHome    { void draw(bool full); void touch(int x, int y); }
namespace ScreenDx      { void draw(bool full); void touch(int x, int y); }
namespace ScreenProp    { void draw(bool full); void touch(int x, int y); }
namespace ScreenAlerts  { void draw(bool full); void touch(int x, int y); }
namespace ScreenSettings{ void draw(bool full); void touch(int x, int y); }

// Helpers shared between screens.
void drawTabBar();
void clearContent();
void drawHeader(const char* title);

// Banner shown when an alert fires.
void maybeShowAlertBanner();

}
