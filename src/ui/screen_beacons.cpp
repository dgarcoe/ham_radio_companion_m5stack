#include "ui_internal.h"

#include "../beacons.h"

namespace Ui { namespace ScreenBeacons {

// Layout: a "now transmitting" panel at the top showing the active station per
// band, a small countdown bar, and a scrollable list of all 18 stations with
// the band each is currently on (or "-" if idle).

static int s_lastSig = -1;

static String shortFreq(int b) {
    char buf[12];
    // Each band's beacon frequency to 3 decimal places (e.g. "14.100").
    snprintf(buf, sizeof(buf), "%.3f", Beacons::kBandMHz[b]);
    return String(buf);
}

static void drawNowPanel() {
    auto& d = M5.Display;
    int x = 6;
    int y = CONTENT_Y + 2;
    int w = SCREEN_W - 12;
    int h = 86;
    drawCard(x, y, w, h);

    d.setFont(&fonts::Font0);
    d.setTextColor(COL_MUTED, COL_CARD);
    d.setTextDatum(top_left);
    d.drawString("NCDXF / IARU - now transmitting", x + 8, y + 4);

    int colW = (w - 16) / Beacons::kBandCount;
    int colY = y + 18;
    for (int b = 0; b < Beacons::kBandCount; b++) {
        int cx = x + 8 + b * colW;
        int idx = Beacons::currentStationIndex(b);

        // Frequency label - small, on top.
        d.setFont(&fonts::Font0);
        d.setTextColor(COL_MUTED, COL_CARD);
        d.setTextDatum(top_center);
        d.drawString(shortFreq(b), cx + colW / 2, colY);

        // Active callsign - Font2 fits a 6-char call inside our ~58px column.
        d.setFont(&fonts::Font2);
        d.setTextColor(COL_ACCENT, COL_CARD);
        d.setTextDatum(top_center);
        d.drawString(idx >= 0 ? Beacons::kStations[idx].call : String("--"),
                     cx + colW / 2, colY + 12);

        // Grid square (always 6 chars) - clean and informative.
        if (idx >= 0) {
            d.setFont(&fonts::Font0);
            d.setTextColor(COL_DIM, COL_CARD);
            d.setTextDatum(top_center);
            d.drawString(Beacons::kStations[idx].grid,
                         cx + colW / 2, colY + 32);
        }
    }

    // 10-second countdown bar at the bottom of the panel.
    int barX = x + 8, barY = y + h - 14, barW = w - 16, barH = 6;
    d.fillRoundRect(barX, barY, barW, barH, 3, COL_BG);
    int rem = Beacons::slotElapsedSeconds();
    if (rem >= 0) {
        int filled = (rem * barW) / 10;
        d.fillRoundRect(barX, barY, filled, barH, 3, COL_ACCENT);
    }
    d.setFont(&fonts::Font0);
    d.setTextColor(COL_DIM, COL_CARD);
    d.setTextDatum(top_right);
    int slotRem = Beacons::slotRemainingSeconds();
    if (slotRem >= 0) {
        d.drawString(String("next slot in ") + slotRem + "s",
                     x + w - 8, barY - 12);
    }
}

static void drawList() {
    auto& d = M5.Display;
    int yTop = CONTENT_Y + 2 + 86 + 4;     // below the "now" card
    int yBot = SCREEN_H - 4;
    int h = yBot - yTop;
    int x = 6, w = SCREEN_W - 12;
    drawCard(x, yTop, w, h);

    d.setFont(&fonts::Font0);
    d.setTextColor(COL_MUTED, COL_CARD);
    d.setTextDatum(top_left);
    d.drawString("CALL", x + 8,   yTop + 4);
    d.drawString("BAND", x + 80,  yTop + 4);
    d.drawString("QTH",  x + 130, yTop + 4);
    d.drawString("GRID", x + w - 50, yTop + 4);

    int rowY = yTop + 16;
    int rowsAvail = (yTop + h - rowY - 2) / 12;
    int rows = Beacons::kStationCount;
    if (rows > rowsAvail) rows = rowsAvail;
    d.setFont(&fonts::Font0);
    for (int i = 0; i < rows; i++) {
        const auto& st = Beacons::kStations[i];
        int ry = rowY + i * 12;
        int activeBand = Beacons::currentBandForStation(i);
        bool active = (activeBand >= 0);
        uint16_t fg = active ? COL_ACCENT : COL_FG;
        d.setTextColor(fg, COL_CARD);
        d.setTextDatum(top_left);
        d.drawString(st.call, x + 8, ry);

        if (active) {
            d.drawString(String(Beacons::kBandLabel[activeBand]) + " MHz",
                         x + 80, ry);
        } else {
            d.setTextColor(COL_MUTED, COL_CARD);
            d.drawString("-", x + 80, ry);
        }

        d.setTextColor(active ? COL_FG : COL_DIM, COL_CARD);
        String c = st.country;
        if (c.length() > 18) c = c.substring(0, 18);
        d.drawString(c, x + 130, ry);

        d.setTextColor(COL_DIM, COL_CARD);
        d.drawString(st.grid, x + w - 50, ry);
    }
}

void draw(bool full) {
    // Compose a coarse signature so we redraw every second when the slot
    // indicator advances, but not more often.
    int sig = Beacons::slotElapsedSeconds() * 31 +
              (Beacons::currentStationIndex(0) << 5);
    if (full) {
        s_lastSig = -1;
    }
    if (sig == s_lastSig && !full) return;
    s_lastSig = sig;

    drawNowPanel();
    drawList();
}

void touch(int /*x*/, int /*y*/) { /* no interactive elements */ }

}}
