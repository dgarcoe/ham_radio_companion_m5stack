#include "ui_internal.h"
#include "icons.h"

#include "../propagation.h"

namespace Ui { namespace ScreenProp {

static String s_lastSig;

static uint16_t condColor(const String& c) {
    String s = c; s.toLowerCase();
    if (s.indexOf("good") >= 0) return COL_OK;
    if (s.indexOf("fair") >= 0) return COL_WARN;
    if (s.indexOf("poor") >= 0) return COL_BAD;
    return COL_DIM;
}

static void drawRefreshButton() {
    auto& d = M5.Display;
    int bw = 88, bh = 22;
    int bx = SCREEN_W - bw - 6;
    int by = CONTENT_Y + 2;
    d.fillRoundRect(bx, by, bw, bh, 4, COL_TAB_SEL);
    // Icon on the left, label on the right.
    Icons::draw(bx + 4, by + 3, Icons::RefreshIcon, COL_FG);
    d.setTextColor(COL_FG, COL_TAB_SEL);
    d.setTextDatum(middle_left);
    d.setFont(&fonts::Font2);
    d.drawString("Refresh", bx + 4 + Icons::W + 4, by + bh / 2);
}

void draw(bool full) {
    auto& d = M5.Display;
    const auto& p = Propagation::data();

    if (full) {
        clearContent();
        drawHeader("Propagation");
        drawRefreshButton();
        s_lastSig = "";
    }

    // Composite signature - only redraw on change.
    String sig = p.valid
        ? (p.solarFlux + "|" + p.aIndex + "|" + p.kIndex + "|" + p.sunspots +
           "|" + p.xrayClass + "|" + p.solarWind + "|" + p.muf + "|" + p.hf +
           "|" + p.signalNoise + "|" + p.aurora + "|" + String((int)p.bands.size()))
        : ("invalid|" + Propagation::status());
    if (sig == s_lastSig) return;
    s_lastSig = sig;

    int y = CONTENT_Y + 30;
    d.fillRect(0, y, SCREEN_W, CONTENT_H - 30, COL_BG);

    if (!p.valid) {
        d.setTextColor(COL_DIM, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.drawString(String("No data: ") + Propagation::status(), SCREEN_W / 2, y + 30);
        d.setTextColor(COL_DIM, COL_BG);
        d.drawString("Tap Refresh after WiFi connects", SCREEN_W / 2, y + 50);
        return;
    }

    // First row: SFI / SN / A / K
    d.setFont(&fonts::Font2);
    d.setTextDatum(top_left);
    auto cell = [&](int cx, int cy, const char* label, const String& val, uint16_t valCol) {
        d.setTextColor(COL_DIM, COL_BG);
        d.drawString(label, cx, cy);
        d.setTextColor(valCol, COL_BG);
        d.setFont(&fonts::Font4);
        d.drawString(val.length() ? val : String("-"), cx, cy + 14);
        d.setFont(&fonts::Font2);
    };

    cell(8,   y,      "SFI",     p.solarFlux, COL_FG);
    cell(85,  y,      "SN",      p.sunspots,  COL_FG);
    cell(160, y,      "A-idx",   p.aIndex,    COL_FG);
    cell(235, y,      "K-idx",   p.kIndex,    COL_FG);

    int y2 = y + 50;
    cell(8,   y2,     "X-Ray",   p.xrayClass, COL_FG);
    cell(85,  y2,     "S/N",     p.signalNoise, COL_FG);
    cell(160, y2,     "MUF",     p.muf,       COL_FG);
    cell(235, y2,     "Aurora",  p.aurora,    COL_FG);

    // Band conditions table
    int y3 = y2 + 50;
    d.setTextColor(COL_DIM, COL_BG);
    d.drawString("Band", 8,  y3);
    d.drawString("Day",  120, y3);
    d.drawString("Night", 200, y3);

    for (size_t i = 0; i < p.bands.size() && i < 4; i++) {
        const auto& b = p.bands[i];
        int ry = y3 + 16 + (int)i * 14;
        d.setTextColor(COL_FG, COL_BG);
        d.drawString(b.band, 8, ry);
        d.setTextColor(condColor(b.dayCond), COL_BG);
        d.drawString(b.dayCond, 120, ry);
        d.setTextColor(condColor(b.nightCond), COL_BG);
        d.drawString(b.nightCond, 200, ry);
    }
}

void touch(int x, int y) {
    int bw = 88, bh = 22;
    int bx = SCREEN_W - bw - 6;
    int by = CONTENT_Y + 2;
    if (x >= bx && x <= bx + bw && y >= by && y <= by + bh) {
        Propagation::fetchNow();
        s_lastSig = ""; // force redraw
    }
}

}}
