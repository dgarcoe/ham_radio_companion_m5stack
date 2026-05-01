#pragma once

#include <M5Unified.h>
#include "ui.h"

namespace Ui {

constexpr int SCREEN_W   = 320;
constexpr int SCREEN_H   = 240;

// Top header on every non-launcher screen: "← Back" + title.
constexpr int HEADER_H   = 30;
constexpr int CONTENT_Y  = HEADER_H;
constexpr int CONTENT_H  = SCREEN_H - HEADER_H;

// Convenience: a sub-header strip inside the content area for action chips.
constexpr int SUBHEADER_H = 30;
constexpr int CONTENT_BODY_Y = CONTENT_Y + SUBHEADER_H;
constexpr int CONTENT_BODY_H = CONTENT_H - SUBHEADER_H;

// Palette.
constexpr uint16_t COL_BG       = 0x10A2;
constexpr uint16_t COL_PANEL    = 0x18C3;
constexpr uint16_t COL_CARD     = 0x2965;
constexpr uint16_t COL_CARD_HI  = 0x39C7;
constexpr uint16_t COL_BORDER   = 0x4208;
constexpr uint16_t COL_FG       = 0xFFFF;
constexpr uint16_t COL_DIM      = 0x9CD3;
constexpr uint16_t COL_MUTED    = 0x6B4D;
constexpr uint16_t COL_ACCENT   = 0x05FF;
constexpr uint16_t COL_ACCENT_D = 0x033D;
constexpr uint16_t COL_OK       = 0x2FE3;
constexpr uint16_t COL_WARN     = 0xFD20;
constexpr uint16_t COL_BAD      = 0xF9C7;
constexpr uint16_t COL_BANNER   = 0xFD20;

// Per-screen draw / touch handlers.
namespace ScreenLauncher{ void draw(bool full); void touch(int x, int y); }
namespace ScreenDx      { void draw(bool full); void touch(int x, int y); }
namespace ScreenProp    { void draw(bool full); void touch(int x, int y); }
namespace ScreenAlerts  { void draw(bool full); void touch(int x, int y); }
namespace ScreenBeacons { void draw(bool full); void touch(int x, int y); }
namespace ScreenPota    { void draw(bool full); void touch(int x, int y); }
namespace ScreenBearing { void draw(bool full); void touch(int x, int y); }
namespace ScreenNoaa    { void draw(bool full); void touch(int x, int y); }
namespace ScreenContests{ void draw(bool full); void touch(int x, int y); }
namespace ScreenGrayline{ void draw(bool full); void touch(int x, int y); }
namespace ScreenSettings{ void draw(bool full); void touch(int x, int y); }

// Geometry helpers shared between screens.
struct Rect {
    int x, y, w, h;
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

// Helpers shared between screens.
void clearContent();                              // fill content area with COL_PANEL
void drawHeader(const char* title);               // back button + title + clock
void drawCard(int x, int y, int w, int h);
void drawCard(int x, int y, int w, int h, uint16_t fill);

Rect drawButton(int x, int y, int w, int h, const char* label,
                uint16_t bg, uint16_t fg);
Rect drawIconButton(int x, int y, int w, int h, const char* label,
                    const char* const* icon, uint16_t bg, uint16_t fg);

// Banner shown when an alert fires.
void maybeShowAlertBanner();

// Hit rect of the back button drawn by drawHeader (only valid on non-launcher
// screens). Provided so screens can early-out before checking other targets.
Rect backButtonRect();

}
