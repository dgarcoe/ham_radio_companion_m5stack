#include "ui_internal.h"
#include "icons.h"

#include "../alerts.h"

namespace Ui {

static Tab s_tab = Tab::Home;
static bool s_needFullRedraw = true;
static uint32_t s_lastPartialRedraw = 0;
static uint32_t s_lastTouchMs = 0;

static const char* kTabLabels[(int)Tab::Count] = {
    "Home", "DX", "Prop", "Alerts", "Set"
};

void drawTabBar() {
    auto& d = M5.Display;
    int n = (int)Tab::Count;
    int w = SCREEN_W / n;

    static Icons::Icon kTabIcons[] = {
        Icons::HomeIcon,
        Icons::DxIcon,
        Icons::PropIcon,
        Icons::AlertsIcon,
        Icons::SettingsIcon,
    };

    for (int i = 0; i < n; i++) {
        int x = i * w;
        bool sel = (i == (int)s_tab);
        uint16_t bg = sel ? COL_TAB_SEL : COL_TAB_BG;
        uint16_t fg = sel ? COL_FG      : COL_DIM;
        d.fillRect(x, 0, w, TAB_H, bg);

        // Icon centered horizontally near the top.
        int iconX = x + (w - Icons::W) / 2;
        int iconY = 2;
        Icons::draw(iconX, iconY, kTabIcons[i], fg);

        // Label below the icon.
        d.setTextColor(fg, bg);
        d.setTextDatum(middle_center);
        d.setFont(&fonts::Font0);
        d.drawString(kTabLabels[i], x + w / 2, TAB_H - 8);

        // Selected-tab accent stripe at the bottom.
        if (sel) d.fillRect(x, TAB_H - 2, w, 2, COL_ACCENT);
    }
    d.drawFastHLine(0, TAB_H, SCREEN_W, COL_DIM);
}

void clearContent() {
    M5.Display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, COL_BG);
}

void drawHeader(const char* title) {
    auto& d = M5.Display;
    d.setTextColor(COL_ACCENT, COL_BG);
    d.setTextDatum(top_left);
    d.setFont(&fonts::Font2);
    d.drawString(title, 6, CONTENT_Y + 4);
    d.drawFastHLine(0, CONTENT_Y + 22, SCREEN_W, COL_DIM);
}

void setTab(Tab t) {
    if (s_tab == t) return;
    s_tab = t;
    s_needFullRedraw = true;
}
Tab currentTab() { return s_tab; }

static void dispatchTouch(int x, int y) {
    if (y < TAB_H) {
        int n = (int)Tab::Count;
        int w = SCREEN_W / n;
        int idx = x / w;
        if (idx >= 0 && idx < n) setTab((Tab)idx);
        return;
    }
    switch (s_tab) {
        case Tab::Home:        ScreenHome::touch(x, y); break;
        case Tab::DxCluster:   ScreenDx::touch(x, y); break;
        case Tab::Propagation: ScreenProp::touch(x, y); break;
        case Tab::Alerts:      ScreenAlerts::touch(x, y); break;
        case Tab::Settings:    ScreenSettings::touch(x, y); break;
        default: break;
    }
}

void begin() {
    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    s_needFullRedraw = true;
}

void loop() {
    auto& d = M5.Display;

    if (s_needFullRedraw) {
        d.fillScreen(COL_BG);
        drawTabBar();
        switch (s_tab) {
            case Tab::Home:        ScreenHome::draw(true); break;
            case Tab::DxCluster:   ScreenDx::draw(true); break;
            case Tab::Propagation: ScreenProp::draw(true); break;
            case Tab::Alerts:      ScreenAlerts::draw(true); break;
            case Tab::Settings:    ScreenSettings::draw(true); break;
            default: break;
        }
        s_needFullRedraw = false;
        s_lastPartialRedraw = millis();
    } else {
        uint32_t now = millis();
        if (now - s_lastPartialRedraw > 1000) {
            s_lastPartialRedraw = now;
            switch (s_tab) {
                case Tab::Home:        ScreenHome::draw(false); break;
                case Tab::DxCluster:   ScreenDx::draw(false); break;
                case Tab::Propagation: ScreenProp::draw(false); break;
                case Tab::Alerts:      ScreenAlerts::draw(false); break;
                case Tab::Settings:    ScreenSettings::draw(false); break;
                default: break;
            }
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
            int by = SCREEN_H - 36;
            d.fillRect(0, by, SCREEN_W, 36, COL_BANNER);
            d.setTextColor(COL_BG, COL_BANNER);
            d.setFont(&fonts::Font2);
            d.setTextDatum(middle_left);
            String line1 = String("ALERT: ") + h.ruleName;
            String line2 = h.spot.dx + " " + String(h.spot.freqKHz, 1) +
                           " kHz " + h.spot.band + " " + h.spot.mode;
            d.drawString(line1, 6, by + 10);
            d.drawString(line2, 6, by + 26);
        }
    } else {
        if (millis() - shownAt > 3500) {
            active = false;
            // Force a full redraw of the current screen so we cover the banner.
            s_needFullRedraw = true;
        }
    }
}

}
