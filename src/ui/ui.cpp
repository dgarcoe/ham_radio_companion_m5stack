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

    // Background strip.
    d.fillRect(0, 0, SCREEN_W, TAB_H, COL_BG);

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

        if (sel) {
            // Selected tab: full-width accent strip on top + slightly lighter card.
            d.fillRect(x + 2, 4, w - 4, TAB_H - 6, COL_CARD_HI);
            d.fillRect(x + 2, 4, w - 4, 3, COL_ACCENT);
        }

        uint16_t fg = sel ? COL_FG : COL_DIM;
        uint16_t bg = sel ? COL_CARD_HI : COL_BG;

        int iconX = x + (w - Icons::W) / 2;
        int iconY = 6;
        Icons::draw(iconX, iconY, kTabIcons[i], fg);

        d.setTextColor(fg, bg);
        d.setTextDatum(middle_center);
        d.setFont(&fonts::Font0);
        d.drawString(kTabLabels[i], x + w / 2, TAB_H - 8);
    }
    // Soft separator between tab bar and content.
    d.drawFastHLine(0, TAB_H, SCREEN_W, COL_BORDER);
}

void clearContent() {
    M5.Display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, COL_PANEL);
}

void drawHeader(const char* title) {
    drawHeader(title, 0);
}

void drawHeader(const char* title, int /*rightActionsW*/) {
    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y, SCREEN_W, HEADER_H, COL_PANEL);
    d.setTextColor(COL_ACCENT, COL_PANEL);
    d.setTextDatum(middle_left);
    d.setFont(&fonts::Font2);
    d.drawString(title, 8, CONTENT_Y + HEADER_H / 2);
    // Hairline separator at the bottom of the header.
    d.drawFastHLine(0, CONTENT_Y + HEADER_H, SCREEN_W, COL_BORDER);
}

void drawCard(int x, int y, int w, int h) {
    drawCard(x, y, w, h, COL_CARD);
}
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

Rect drawChip(int x, int y, const char* label, bool selected) {
    auto& d = M5.Display;
    d.setFont(&fonts::Font0);
    int textW = d.textWidth(label);
    int w = textW + 14;
    int h = 18;
    uint16_t bg = selected ? COL_ACCENT_D : COL_CARD;
    uint16_t fg = selected ? COL_FG       : COL_DIM;
    d.fillRoundRect(x, y, w, h, 9, bg);
    d.drawRoundRect(x, y, w, h, 9, selected ? COL_ACCENT : COL_BORDER);
    d.setTextColor(fg, bg);
    d.setTextDatum(middle_center);
    d.drawString(label, x + w / 2, y + h / 2);
    return { x, y, w, h };
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
        clearContent();
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
            // Force a full redraw of the current screen so we cover the banner.
            s_needFullRedraw = true;
        }
    }
}

}
