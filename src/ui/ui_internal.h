#pragma once

#include <M5Unified.h>
#include "ui.h"

namespace Ui {

// Layout constants for the touch UI.
constexpr int SCREEN_W   = 320;
constexpr int SCREEN_H   = 240;
constexpr int TAB_H      = 40;
constexpr int CONTENT_Y  = TAB_H;
constexpr int CONTENT_H  = SCREEN_H - TAB_H;

// Header strip is the top of the content area: title on the left, optional
// action chips/buttons on the right. Body content starts at CONTENT_BODY_Y.
constexpr int HEADER_H   = 30;
constexpr int CONTENT_BODY_Y = CONTENT_Y + HEADER_H;
constexpr int CONTENT_BODY_H = CONTENT_H - HEADER_H;

// "Modern" palette (RGB565). Dark slate background with cyan accents.
constexpr uint16_t COL_BG       = 0x10A2;     // very dark slate blue
constexpr uint16_t COL_PANEL    = 0x18C3;     // slightly lighter than bg, used as page bg
constexpr uint16_t COL_CARD     = 0x2965;     // card surface
constexpr uint16_t COL_CARD_HI  = 0x39C7;     // selected/hover card
constexpr uint16_t COL_BORDER   = 0x4208;     // hairline border
constexpr uint16_t COL_FG       = 0xFFFF;     // primary text
constexpr uint16_t COL_DIM      = 0x9CD3;     // secondary text
constexpr uint16_t COL_MUTED    = 0x6B4D;     // tertiary text
constexpr uint16_t COL_ACCENT   = 0x05FF;     // cyan
constexpr uint16_t COL_ACCENT_D = 0x033D;     // darker cyan
constexpr uint16_t COL_OK       = 0x2FE3;     // green
constexpr uint16_t COL_WARN     = 0xFD20;     // orange
constexpr uint16_t COL_BAD      = 0xF9C7;     // red
constexpr uint16_t COL_BANNER   = 0xFD20;

// Per-screen draw / touch handlers.
namespace ScreenHome    { void draw(bool full); void touch(int x, int y); }
namespace ScreenDx      { void draw(bool full); void touch(int x, int y); }
namespace ScreenProp    { void draw(bool full); void touch(int x, int y); }
namespace ScreenAlerts  { void draw(bool full); void touch(int x, int y); }
namespace ScreenSettings{ void draw(bool full); void touch(int x, int y); }

// Helpers shared between screens.
void drawTabBar();
void clearContent();                              // fills content area with COL_PANEL
void drawHeader(const char* title);               // header strip with title only
void drawHeader(const char* title,
                int rightActionsW);               // header with reserved space on the right
void drawCard(int x, int y, int w, int h);
void drawCard(int x, int y, int w, int h, uint16_t fill);

// A pill button (rounded rect) that returns the rect via outRect for hit testing.
struct Rect { int x, y, w, h; bool contains(int px, int py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
}};
Rect drawButton(int x, int y, int w, int h, const char* label,
                uint16_t bg, uint16_t fg);
Rect drawIconButton(int x, int y, int w, int h, const char* label,
                    const char* const* icon /* 16x16, may be null */,
                    uint16_t bg, uint16_t fg);

// Filter chip used in the DX screen.
Rect drawChip(int x, int y, const char* label, bool selected);

// Banner shown when an alert fires.
void maybeShowAlertBanner();

}
