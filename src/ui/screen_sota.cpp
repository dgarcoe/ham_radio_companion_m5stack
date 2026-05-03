#include "ui_internal.h"
#include "icons.h"

#include <algorithm>
#include "../sota.h"

namespace Ui { namespace ScreenSota {

static int s_scroll = 0;
static int s_lastSig = -1;
static Rect s_btnRefresh { 0, 0, 0, 0 };
static Rect s_btnUp      { 0, 0, 0, 0 };
static Rect s_btnDown    { 0, 0, 0, 0 };
static const int kRowH = 30;
static int s_visibleRows = 0;

static int subY() { return CONTENT_Y + 2; }

static void drawSubHeader() {
    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y, SCREEN_W, SUBHEADER_H, COL_PANEL);

    d.setFont(&fonts::Font2);
    d.setTextColor(COL_DIM, COL_PANEL);
    d.setTextDatum(middle_left);
    String left = String((unsigned)Sota::spotCount()) + " spots";
    uint32_t age = Sota::lastFetchAgeSeconds();
    if (age != UINT32_MAX) {
        char ageBuf[16];
        if (age < 60)        snprintf(ageBuf, sizeof(ageBuf), " - %lus ago", (unsigned long)age);
        else if (age < 3600) snprintf(ageBuf, sizeof(ageBuf), " - %lum ago", (unsigned long)(age / 60));
        else                 snprintf(ageBuf, sizeof(ageBuf), " - %luh ago", (unsigned long)(age / 3600));
        left += ageBuf;
    } else {
        left += String(" - ") + Sota::status();
    }
    d.drawString(left, 8, subY() + 11);

    int bw = 96, bh = 22;
    int bx = SCREEN_W - bw - 8;
    int by = CONTENT_Y + (SUBHEADER_H - bh) / 2;
    s_btnRefresh = drawIconButton(bx, by, bw, bh, "Refresh",
                                  Icons::RefreshIcon, COL_ACCENT_D, COL_FG);
}

static int listRightCol() { return SCREEN_W - 30; }

static void drawScrollButtons() {
    int x = listRightCol() + 4;
    int wBtn = 22, hBtn = 26;
    s_btnUp   = drawButton(x, CONTENT_BODY_Y + 4,          wBtn, hBtn, "^", COL_CARD, COL_FG);
    s_btnDown = drawButton(x, SCREEN_H - hBtn - 4,         wBtn, hBtn, "v", COL_CARD, COL_FG);
}

static void drawList() {
    auto& d = M5.Display;
    int x0 = 0, y0 = CONTENT_BODY_Y;
    int w0 = listRightCol(), h0 = SCREEN_H - CONTENT_BODY_Y;
    d.fillRect(x0, y0, w0, h0, COL_PANEL);

    s_visibleRows = h0 / kRowH;
    auto& spots = Sota::spots();

    if (spots.empty()) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        String msg = (Sota::lastFetchAgeSeconds() == UINT32_MAX)
            ? Sota::status()
            : String("No spots right now.");
        d.drawString(msg, SCREEN_W / 2, y0 + h0 / 2);
        return;
    }

    int start = std::max(0, std::min(s_scroll, (int)spots.size() - 1));

    for (int i = 0; i < s_visibleRows; i++) {
        int idx = start + i;
        if (idx >= (int)spots.size()) break;
        const SotaSpot& sp = spots[idx];
        int y = y0 + i * kRowH;
        uint16_t bg = (i & 1) ? COL_BG : COL_PANEL;
        d.fillRect(0, y, w0, kRowH, bg);

        // Top line: summit ref | activator | freq | mode
        int topMid = y + 9;
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_ACCENT, bg);
        d.drawString(sp.summit.substring(0, 10), 6, topMid);
        d.setTextColor(COL_FG, bg);
        d.drawString(sp.activator.substring(0, 10), 90, topMid);
        if (sp.freqKHz > 0) {
            char freq[12];
            snprintf(freq, sizeof(freq), "%.1f", sp.freqKHz);
            d.drawString(freq, 175, topMid);
        }
        d.setTextColor(COL_WARN, bg);
        d.drawString(sp.mode.substring(0, 4), 245, topMid);

        // Bottom line: summit name
        int botMid = y + 22;
        d.setFont(&fonts::Font0);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_DIM, bg);
        String name = sp.summitName;
        if (name.length() > 46) name = name.substring(0, 46);
        d.drawString(name, 6, botMid);
    }
}

void draw(bool full) {
    if (full) s_lastSig = -1;

    int sig = (int)Sota::spotCount() * 131 +
              (int)(Sota::lastFetchAgeSeconds() / 5) +
              s_scroll * 7;
    if (sig == s_lastSig && !full) return;
    s_lastSig = sig;

    drawSubHeader();
    drawScrollButtons();
    drawList();
}

void touch(int x, int y) {
    if (s_btnRefresh.contains(x, y)) {
        Sota::fetchNow();
        s_lastSig = -1;
        return;
    }
    int rows = std::max(1, s_visibleRows);
    if (s_btnUp.contains(x, y)) {
        s_scroll = std::max(0, s_scroll - rows);
        s_lastSig = -1;
    } else if (s_btnDown.contains(x, y)) {
        s_scroll += rows;
        s_lastSig = -1;
    }
}

}}
