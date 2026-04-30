#include "ui_internal.h"

#include "../dx_cluster.h"

namespace Ui { namespace ScreenDx {

static int s_scroll = 0;            // top-of-list spot index
static const int kRowH = 18;
static int s_rows = 0;
static size_t s_lastDrawnCount = 0;
static int s_lastDrawnScroll = -1;

static void drawScrollControls() {
    auto& d = M5.Display;
    int yTop = CONTENT_Y + 24;
    int yBot = SCREEN_H - 24;
    d.fillRect(SCREEN_W - 24, yTop, 24, 20, COL_TAB_BG);
    d.fillRect(SCREEN_W - 24, yBot, 24, 20, COL_TAB_BG);
    d.setTextColor(COL_FG, COL_TAB_BG);
    d.setFont(&fonts::Font2);
    d.setTextDatum(middle_center);
    d.drawString("^", SCREEN_W - 12, yTop + 10);
    d.drawString("v", SCREEN_W - 12, yBot + 10);
}

void draw(bool full) {
    auto& d = M5.Display;
    auto& spots = DxCluster::spots();

    if (full) {
        clearContent();
        drawHeader("DX Cluster");
        drawScrollControls();
        s_rows = (CONTENT_H - 30) / kRowH;
        s_lastDrawnCount = (size_t)-1;
        s_lastDrawnScroll = -1;
    }

    // Only redraw the list if data or scroll changed.
    if (spots.size() == s_lastDrawnCount && s_scroll == s_lastDrawnScroll) return;
    s_lastDrawnCount = spots.size();
    s_lastDrawnScroll = s_scroll;

    int listX = 0;
    int listY = CONTENT_Y + 24;
    int listW = SCREEN_W - 26;
    int listH = s_rows * kRowH;
    d.fillRect(listX, listY, listW, listH, COL_BG);

    if (spots.empty()) {
        d.setTextColor(COL_DIM, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.drawString(DxCluster::status(), SCREEN_W / 2, listY + listH / 2);
        return;
    }

    // Header row
    d.fillRect(0, listY, listW, kRowH, COL_TAB_BG);
    d.setTextColor(COL_DIM, COL_TAB_BG);
    d.setFont(&fonts::Font0);
    d.setTextDatum(top_left);
    d.drawString("Band", 4,   listY + 4);
    d.drawString("Freq", 44,  listY + 4);
    d.drawString("DX",   100, listY + 4);
    d.drawString("Mode", 190, listY + 4);
    d.drawString("Age",  240, listY + 4);

    int rows = s_rows - 1;
    if (rows < 1) rows = 1;
    int start = s_scroll;
    if (start < 0) start = 0;
    if (start > (int)spots.size() - 1) start = spots.size() - 1;

    uint32_t now = millis();
    for (int i = 0; i < rows; i++) {
        int idx = start + i;
        if (idx >= (int)spots.size()) break;
        const DxSpot& sp = spots[idx];
        int y = listY + kRowH + i * kRowH;

        d.setFont(&fonts::Font2);
        d.setTextDatum(top_left);
        d.setTextColor(COL_ACCENT, COL_BG);
        d.drawString(sp.band, 4, y + 1);

        d.setTextColor(COL_FG, COL_BG);
        char freqBuf[16];
        snprintf(freqBuf, sizeof(freqBuf), "%.1f", sp.freqKHz);
        d.drawString(freqBuf, 44, y + 1);

        d.drawString(sp.dx.substring(0, 11), 100, y + 1);

        d.setTextColor(COL_WARN, COL_BG);
        d.drawString(sp.mode.substring(0, 4), 190, y + 1);

        uint32_t ageS = (now - sp.rxMillis) / 1000;
        char ageBuf[8];
        if (ageS < 60)         snprintf(ageBuf, sizeof(ageBuf), "%lus", (unsigned long)ageS);
        else if (ageS < 3600)  snprintf(ageBuf, sizeof(ageBuf), "%lum", (unsigned long)(ageS / 60));
        else                   snprintf(ageBuf, sizeof(ageBuf), "%luh", (unsigned long)(ageS / 3600));
        d.setTextColor(COL_DIM, COL_BG);
        d.drawString(ageBuf, 240, y + 1);
    }
}

void touch(int x, int y) {
    auto& spots = DxCluster::spots();
    int rows = s_rows - 1;
    if (rows < 1) rows = 1;

    if (x > SCREEN_W - 24) {
        if (y < CONTENT_Y + 60) {
            s_scroll = max(0, s_scroll - rows);
        } else if (y > SCREEN_H - 30) {
            s_scroll = min(max(0, (int)spots.size() - 1), s_scroll + rows);
        }
    }
}

}}
