#include "ui_internal.h"
#include "icons.h"

#include <algorithm>
#include "../noaa.h"

namespace Ui { namespace ScreenNoaa {

static int s_scroll = 0;
static int s_lastSig = -1;
static Rect s_btnRefresh { 0, 0, 0, 0 };
static Rect s_btnUp      { 0, 0, 0, 0 };
static Rect s_btnDown    { 0, 0, 0, 0 };
static const int kRowH = 34;
static int s_visibleRows = 0;

static uint16_t severityColor(const String& code) {
    // SWPC product codes: ALT=alert, WAR=warning, WAT=watch, SUM=summary
    String c = code;
    c.toUpperCase();
    if (c.startsWith("WAR")) return COL_BAD;     // warning
    if (c.startsWith("ALT")) return COL_WARN;    // alert
    if (c.startsWith("WAT")) return COL_WARN;    // watch
    return COL_DIM;
}

static void drawSubHeader() {
    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y, SCREEN_W, SUBHEADER_H, COL_PANEL);

    d.setFont(&fonts::Font2);
    d.setTextColor(COL_DIM, COL_PANEL);
    d.setTextDatum(middle_left);
    String left = String((unsigned)Noaa::count()) + " alerts";
    uint32_t age = Noaa::lastFetchAgeSeconds();
    if (age != UINT32_MAX) {
        char ageBuf[24];
        if (age < 60)        snprintf(ageBuf, sizeof(ageBuf), " - %lus ago", (unsigned long)age);
        else if (age < 3600) snprintf(ageBuf, sizeof(ageBuf), " - %lum ago", (unsigned long)(age / 60));
        else                 snprintf(ageBuf, sizeof(ageBuf), " - %luh ago", (unsigned long)(age / 3600));
        left += ageBuf;
    } else {
        left += String(" - ") + Noaa::status();
    }
    d.drawString(left, 8, CONTENT_Y + SUBHEADER_H / 2);

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
    auto& alerts = Noaa::alerts();

    if (alerts.empty()) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        String msg = (Noaa::lastFetchAgeSeconds() == UINT32_MAX)
            ? Noaa::status()
            : String("No active alerts.");
        d.drawString(msg, SCREEN_W / 2, y0 + h0 / 2);
        return;
    }

    int start = s_scroll;
    if (start < 0) start = 0;
    if (start > (int)alerts.size() - 1) start = (int)alerts.size() - 1;

    for (int i = 0; i < s_visibleRows; i++) {
        int idx = start + i;
        if (idx >= (int)alerts.size()) break;
        const NoaaAlert& a = alerts[idx];
        int y = y0 + i * kRowH;
        uint16_t bg = (i & 1) ? COL_BG : COL_PANEL;
        d.fillRect(0, y, w0, kRowH, bg);

        // Top line: code (severity-colored) + issued timestamp (truncated).
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        d.setTextColor(severityColor(a.code), bg);
        d.drawString(a.code, 6, y + 10);

        d.setTextColor(COL_DIM, bg);
        // Show date+time but trim the fractional seconds.
        String ts = a.issued;
        int dot = ts.indexOf('.');
        if (dot > 0) ts = ts.substring(0, dot);
        if (ts.length() > 19) ts = ts.substring(0, 19);
        d.setTextDatum(middle_right);
        d.drawString(ts, w0 - 6, y + 10);

        // Bottom line: summary in dim grey.
        d.setFont(&fonts::Font0);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_FG, bg);
        String summary = a.summary;
        if (summary.length() > 50) summary = summary.substring(0, 50);
        d.drawString(summary, 6, y + 24);
    }
}

void draw(bool full) {
    if (full) s_lastSig = -1;

    int sig = (int)Noaa::count() * 131
            + (int)(Noaa::lastFetchAgeSeconds() / 5)
            + s_scroll * 7;
    if (sig == s_lastSig && !full) return;
    s_lastSig = sig;

    drawSubHeader();
    drawScrollButtons();
    drawList();
}

void touch(int x, int y) {
    if (s_btnRefresh.contains(x, y)) {
        Noaa::fetchNow();
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
