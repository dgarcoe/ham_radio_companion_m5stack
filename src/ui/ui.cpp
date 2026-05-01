#include "ui_internal.h"
#include "icons.h"

#include <time.h>
#include "../alerts.h"

namespace Ui {

static Screen s_screen = Screen::Launcher;
static bool s_needFullRedraw = true;
static uint32_t s_lastPartialRedraw = 0;
static uint32_t s_lastTouchMs = 0;
static Rect s_backRect { 0, 0, 0, 0 };

static const char* screenTitle(Screen s) {
    switch (s) {
        case Screen::DxCluster:   return "DX Cluster";
        case Screen::Propagation: return "Propagation";
        case Screen::Alerts:      return "Alerts";
        case Screen::Beacons:     return "NCDXF Beacons";
        case Screen::Pota:        return "POTA";
        case Screen::Bearing:     return "Bearing & Distance";
        case Screen::Noaa:        return "Space Weather";
        case Screen::Contests:    return "Contest Calendar";
        case Screen::Grayline:    return "Grayline Map";
        case Screen::Settings:    return "Settings";
        default:                  return "";
    }
}

void clearContent() {
    M5.Display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, COL_PANEL);
}

void drawHeader(const char* title) {
    auto& d = M5.Display;
    d.fillRect(0, 0, SCREEN_W, HEADER_H, COL_BG);

    // Back button (rounded card with arrow).
    int bw = 56, bh = 24;
    int by = (HEADER_H - bh) / 2;
    int bx = 4;
    d.fillRoundRect(bx, by, bw, bh, 5, COL_CARD);
    d.drawRoundRect(bx, by, bw, bh, 5, COL_BORDER);
    d.setTextColor(COL_FG, COL_CARD);
    d.setFont(&fonts::Font2);
    d.setTextDatum(middle_center);
    d.drawString("< Home", bx + bw / 2, by + bh / 2);
    s_backRect = { bx, by, bw, bh };

    // Title centered on the rest.
    d.setTextColor(COL_ACCENT, COL_BG);
    d.setTextDatum(middle_left);
    d.drawString(title, bx + bw + 12, HEADER_H / 2);

    // Mini-clock on the right.
    time_t now = time(nullptr);
    if (now > 1700000000) {
        struct tm tmv;
        gmtime_r(&now, &tmv);
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02dZ", tmv.tm_hour, tmv.tm_min);
        d.setTextColor(COL_DIM, COL_BG);
        d.setTextDatum(middle_right);
        d.drawString(buf, SCREEN_W - 8, HEADER_H / 2);
    }

    d.drawFastHLine(0, HEADER_H, SCREEN_W, COL_BORDER);
}

Rect backButtonRect() { return s_backRect; }

void drawCard(int x, int y, int w, int h) { drawCard(x, y, w, h, COL_CARD); }
void drawCard(int x, int y, int w, int h, uint16_t fill) {
    auto& d = M5.Display;
    d.fillRoundRect(x, y, w, h, 6, fill);
    d.drawRoundRect(x, y, w, h, 6, COL_BORDER);
}

Rect drawButton(int x, int y, int w, int h, const char* label,
                uint16_t bg, uint16_t fg) {
    auto& d = M5.Display;
    d.fillRoundRect(x, y, w, h, 5, bg);
    d.drawRoundRect(x, y, w, h, 5, COL_BORDER);
    d.setTextColor(fg, bg);
    d.setTextDatum(middle_center);
    d.setFont(&fonts::Font2);
    d.drawString(label, x + w / 2, y + h / 2);
    return { x, y, w, h };
}

Rect drawIconButton(int x, int y, int w, int h, const char* label,
                    const char* const* icon, uint16_t bg, uint16_t fg) {
    auto& d = M5.Display;
    d.fillRoundRect(x, y, w, h, 5, bg);
    d.drawRoundRect(x, y, w, h, 5, COL_BORDER);
    int textX = x + 8;
    if (icon) {
        Icons::draw(x + 6, y + (h - Icons::H) / 2, icon, fg);
        textX = x + 6 + Icons::W + 4;
    }
    d.setTextColor(fg, bg);
    d.setTextDatum(middle_left);
    d.setFont(&fonts::Font2);
    d.drawString(label, textX, y + h / 2);
    return { x, y, w, h };
}

void setScreen(Screen s) {
    if (s_screen == s) return;
    s_screen = s;
    s_needFullRedraw = true;
}
Screen currentScreen() { return s_screen; }
void goHome() { setScreen(Screen::Launcher); }
void requestFullRedraw() { s_needFullRedraw = true; }

static void drawCurrentScreenChrome() {
    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    if (s_screen == Screen::Launcher) {
        s_backRect = { 0, 0, 0, 0 };
        // Launcher draws its own top banner; nothing else to do here.
    } else {
        drawHeader(screenTitle(s_screen));
        clearContent();
    }
}

static void dispatchDraw(bool full) {
    switch (s_screen) {
        case Screen::Launcher:    ScreenLauncher::draw(full); break;
        case Screen::DxCluster:   ScreenDx::draw(full); break;
        case Screen::Propagation: ScreenProp::draw(full); break;
        case Screen::Alerts:      ScreenAlerts::draw(full); break;
        case Screen::Beacons:     ScreenBeacons::draw(full); break;
        case Screen::Pota:        ScreenPota::draw(full); break;
        case Screen::Bearing:     ScreenBearing::draw(full); break;
        case Screen::Noaa:        ScreenNoaa::draw(full); break;
        case Screen::Contests:    ScreenContests::draw(full); break;
        case Screen::Grayline:    ScreenGrayline::draw(full); break;
        case Screen::Settings:    ScreenSettings::draw(full); break;
        default: break;
    }
}

static void dispatchTouch(int x, int y) {
    if (s_screen != Screen::Launcher && s_backRect.contains(x, y)) {
        goHome();
        return;
    }
    switch (s_screen) {
        case Screen::Launcher:    ScreenLauncher::touch(x, y); break;
        case Screen::DxCluster:   ScreenDx::touch(x, y); break;
        case Screen::Propagation: ScreenProp::touch(x, y); break;
        case Screen::Alerts:      ScreenAlerts::touch(x, y); break;
        case Screen::Beacons:     ScreenBeacons::touch(x, y); break;
        case Screen::Pota:        ScreenPota::touch(x, y); break;
        case Screen::Bearing:     ScreenBearing::touch(x, y); break;
        case Screen::Noaa:        ScreenNoaa::touch(x, y); break;
        case Screen::Contests:    ScreenContests::touch(x, y); break;
        case Screen::Grayline:    ScreenGrayline::touch(x, y); break;
        case Screen::Settings:    ScreenSettings::touch(x, y); break;
        default: break;
    }
}

void begin() {
    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    s_needFullRedraw = true;
}

void loop() {
    if (s_needFullRedraw) {
        drawCurrentScreenChrome();
        dispatchDraw(true);
        s_needFullRedraw = false;
        s_lastPartialRedraw = millis();
    } else {
        uint32_t now = millis();
        if (now - s_lastPartialRedraw > 1000) {
            s_lastPartialRedraw = now;
            // Header mini-clock refresh on non-launcher screens.
            if (s_screen != Screen::Launcher) {
                auto& d = M5.Display;
                d.fillRect(SCREEN_W - 70, 0, 68, HEADER_H - 1, COL_BG);
                time_t now2 = time(nullptr);
                if (now2 > 1700000000) {
                    struct tm tmv;
                    gmtime_r(&now2, &tmv);
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%02d:%02dZ", tmv.tm_hour, tmv.tm_min);
                    d.setTextColor(COL_DIM, COL_BG);
                    d.setFont(&fonts::Font2);
                    d.setTextDatum(middle_right);
                    d.drawString(buf, SCREEN_W - 8, HEADER_H / 2);
                }
            }
            dispatchDraw(false);
        }
    }

    auto t = M5.Touch.getDetail();
    if (t.wasPressed()) {
        uint32_t now = millis();
        if (now - s_lastTouchMs > 150) {
            s_lastTouchMs = now;
            dispatchTouch(t.x, t.y);
        }
    }

    // Hardware Button A on Core2 returns to the launcher.
    if (M5.BtnA.wasPressed() && s_screen != Screen::Launcher) {
        goHome();
    }

    maybeShowAlertBanner();
}

void maybeShowAlertBanner() {
    static AlertHit current;
    static uint32_t shownAt = 0;
    static bool active = false;

    if (!active) {
        AlertHit h;
        if (Alerts::consumeBanner(h)) {
            current = h;
            active = true;
            shownAt = millis();
            auto& d = M5.Display;
            int by = SCREEN_H - 40;
            d.fillRoundRect(6, by, SCREEN_W - 12, 36, 6, COL_BANNER);
            d.drawRoundRect(6, by, SCREEN_W - 12, 36, 6, COL_BORDER);
            d.setTextColor(COL_BG, COL_BANNER);
            d.setFont(&fonts::Font2);
            d.setTextDatum(middle_left);
            String line1 = String("ALERT: ") + h.ruleName;
            String line2 = h.spot.dx + " " + String(h.spot.freqKHz, 1) +
                           " kHz " + h.spot.band + " " + h.spot.mode;
            d.drawString(line1, 14, by + 11);
            d.drawString(line2, 14, by + 26);
        }
    } else {
        if (millis() - shownAt > 3500) {
            active = false;
            s_needFullRedraw = true;
        }
    }
}

}
