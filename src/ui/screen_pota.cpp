#include "ui_internal.h"
#include "icons.h"

#include <algorithm>
#include "../pota.h"

namespace Ui { namespace ScreenPota {

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
    int y = subY();
    d.fillRect(0, CONTENT_Y, SCREEN_W, SUBHEADER_H, COL_PANEL);

    // Left side: count + age status.
    d.setFont(&fonts::Font2);
    d.setTextColor(COL_DIM, COL_PANEL);
    d.setTextDatum(middle_left);
    String left = String((unsigned)Pota::spotCount()) + " spots";
    uint32_t age = Pota::lastFetchAgeSeconds();
    if (age != UINT32_MAX) {
        char ageBuf[16];
        if (age < 60)        snprintf(ageBuf, sizeof(ageBuf), " - %lus ago", (unsigned long)age);
        else if (age < 3600) snprintf(ageBuf, sizeof(ageBuf), " - %lum ago", (unsigned long)(age / 60));
        else                 snprintf(ageBuf, sizeof(ageBuf), " - %luh ago", (unsigned long)(age / 3600));
        left += ageBuf;
    } else {
        left += String(" - ") + Pota::status();
    }
    d.drawString(left, 8, y + 11);

    // Right side: Refresh button.
    int bw = 96, bh = 22;
    int bx = SCREEN_W - bw - 8;
    int by = CONTENT_Y + (SUBHEADER_H - bh) / 2;
    s_btnRefresh = drawIconButton(bx, by, bw, bh, "Refresh",
                                  Icons::RefreshIcon, COL_ACCENT_D, COL_FG);
}

static int listRightCol() { return SCREEN_W - 30; }

static void drawScrollButtons() {
    int x = listRightCol() + 4;
    int wBtn = 22;
    int hBtn = 26;
    int yTop = CONTENT_BODY_Y + 4;
    int yBot = SCREEN_H - hBtn - 4;
    s_btnUp   = drawButton(x, yTop, wBtn, hBtn, "^", COL_CARD, COL_FG);
    s_btnDown = drawButton(x, yBot, wBtn, hBtn, "v", COL_CARD, COL_FG);
}

static void drawList() {
    auto& d = M5.Display;
    int x0 = 0, y0 = CONTENT_BODY_Y;
    int w0 = listRightCol(), h0 = SCREEN_H - CONTENT_BODY_Y;
    d.fillRect(x0, y0, w0, h0, COL_PANEL);

    s_visibleRows = h0 / kRowH;
    auto& spots = Pota::spots();

    if (spots.empty()) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        String msg = (Pota::lastFetchAgeSeconds() == UINT32_MAX)
            ? Pota::status()
            : String("No spots right now.");
        d.drawString(msg, SCREEN_W / 2, y0 + h0 / 2);
        return;
    }

    int start = s_scroll;
    if (start < 0) start = 0;
    if (start > (int)spots.size() - 1) start = (int)spots.size() - 1;

    for (int i = 0; i < s_visibleRows; i++) {
        int idx = start + i;
        if (idx >= (int)spots.size()) break;
        const PotaSpot& sp = spots[idx];
        int y = y0 + i * kRowH;
        uint16_t bg = (i & 1) ? COL_BG : COL_PANEL;
        d.fillRect(0, y, w0, kRowH, bg);

        // Top line (Font2, ~16px tall) - reference + activator + freq + mode.
        // Drawn with middle-left datum so it sits cleanly above the second line.
        int topMid = y + 9;
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_ACCENT, bg);
        d.drawString(sp.reference, 6, topMid);
        d.setTextColor(COL_FG, bg);
        d.drawString(sp.activator.substring(0, 12), 80, topMid);
        char freq[16];
        snprintf(freq, sizeof(freq), "%.1f", sp.freqKHz);
        d.drawString(freq, 175, topMid);
        d.setTextColor(COL_WARN, bg);
        d.drawString(sp.mode.substring(0, 4), 240, topMid);

        // Bottom line (Font0, ~8px tall) - park name + location code. Sits
        // *below* the top line; previously they overlapped because Font2
        // extended to y+18 while this was anchored at y+14.
        int botMid = y + 22;
        d.setFont(&fonts::Font0);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_DIM, bg);
        String parkLine = sp.parkName;
        if (sp.location.length()) {
            if (parkLine.length()) parkLine += "  ";
            parkLine += "[" + sp.location + "]";
        }
        // Trim hard to keep within the list area (~44 chars at Font0 width 6).
        if (parkLine.length() > 44) parkLine = parkLine.substring(0, 44);
        d.drawString(parkLine, 6, botMid);
    }
}

void draw(bool full) {
    if (full) {
        s_lastSig = -1;
    }

    int sig = (int)Pota::spotCount() * 131 +
              (int)(Pota::lastFetchAgeSeconds() / 5) +
              s_scroll * 7;
    if (sig == s_lastSig && !full) return;
    s_lastSig = sig;

    drawSubHeader();
    drawScrollButtons();
    drawList();
}

void touch(int x, int y) {
    if (s_btnRefresh.contains(x, y)) {
        Pota::fetchNow();
        s_lastSig = -1;
        return;
    }
    int rows = s_visibleRows;
    if (rows < 1) rows = 1;
    if (s_btnUp.contains(x, y)) {
        s_scroll = std::max(0, s_scroll - rows);
        s_lastSig = -1;
    } else if (s_btnDown.contains(x, y)) {
        s_scroll += rows;
        s_lastSig = -1;
    }
}

}}
