#include "ui_internal.h"

#include <algorithm>
#include <time.h>
#include "../contests.h"

namespace Ui { namespace ScreenContests {

static int s_scroll = 0;
static int s_lastSig = -1;
static const int kRowH = 36;
static int s_visibleRows = 0;
static Rect s_btnUp   { 0, 0, 0, 0 };
static Rect s_btnDown { 0, 0, 0, 0 };

static int listRightCol() { return SCREEN_W - 30; }

static String formatStart(time_t t) {
    struct tm tmv;
    gmtime_r(&t, &tmv);
    static const char* kMonths[] = {
        "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
    };
    static const char* kDays[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
    char buf[24];
    snprintf(buf, sizeof(buf), "%s %s %d  %02d:%02dZ",
             kDays[tmv.tm_wday], kMonths[tmv.tm_mon], tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min);
    return String(buf);
}

static String relativeStart(time_t startUtc, time_t endUtc, time_t now) {
    if (now >= startUtc && now < endUtc) {
        long left = (long)(endUtc - now);
        char b[24];
        if (left < 3600)        snprintf(b, sizeof(b), "RUNNING - %ldm left", left / 60);
        else if (left < 86400)  snprintf(b, sizeof(b), "RUNNING - %ldh left", left / 3600);
        else                    snprintf(b, sizeof(b), "RUNNING - %ldd left", left / 86400);
        return String(b);
    }
    long delta = (long)(startUtc - now);
    char b[24];
    if (delta < 3600)        snprintf(b, sizeof(b), "starts in %ldm", delta / 60);
    else if (delta < 86400)  snprintf(b, sizeof(b), "starts in %ldh", delta / 3600);
    else if (delta < 7 * 86400) snprintf(b, sizeof(b), "starts in %ldd", delta / 86400);
    else                     snprintf(b, sizeof(b), "starts in %ldd", delta / 86400);
    return String(b);
}

static uint16_t modeColor(const String& mode) {
    if (mode == "CW")    return COL_ACCENT;
    if (mode == "SSB")   return COL_OK;
    if (mode == "RTTY")  return COL_WARN;
    return COL_DIM;
}

static void drawSubHeader() {
    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y, SCREEN_W, SUBHEADER_H, COL_PANEL);

    d.setFont(&fonts::Font2);
    d.setTextColor(COL_DIM, COL_PANEL);
    d.setTextDatum(middle_left);
    String left;
    if (!Contests::ready()) {
        left = "waiting for time sync...";
    } else {
        left = String((unsigned)Contests::upcoming().size()) + " upcoming";
    }
    d.drawString(left, 8, CONTENT_Y + SUBHEADER_H / 2);
}

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
    auto& list = Contests::upcoming();

    if (list.empty()) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.drawString(Contests::ready() ? "No upcoming contests" : "Waiting for time sync...",
                     SCREEN_W / 2, y0 + h0 / 2);
        return;
    }

    time_t now = time(nullptr);
    int start = s_scroll;
    if (start < 0) start = 0;
    if (start > (int)list.size() - 1) start = (int)list.size() - 1;

    for (int i = 0; i < s_visibleRows; i++) {
        int idx = start + i;
        if (idx >= (int)list.size()) break;
        const Contest& c = list[idx];
        int y = y0 + i * kRowH;
        uint16_t bg = (i & 1) ? COL_BG : COL_PANEL;
        d.fillRect(0, y, w0, kRowH, bg);

        // Top line: name + mode badge.
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_FG, bg);
        // Truncate name to fit before the mode badge.
        String name = c.name;
        if (name.length() > 24) name = name.substring(0, 24);
        d.drawString(name, 8, y + 11);

        // Mode badge on the right of top line.
        d.setTextDatum(middle_right);
        d.setTextColor(modeColor(c.mode), bg);
        d.drawString(c.mode, w0 - 8, y + 11);

        // Bottom line: date + relative time.
        d.setFont(&fonts::Font0);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_DIM, bg);
        d.drawString(formatStart(c.startUtc), 8, y + 26);

        bool running = (now >= c.startUtc && now < c.endUtc);
        d.setTextColor(running ? COL_OK : COL_MUTED, bg);
        d.setTextDatum(middle_right);
        d.drawString(relativeStart(c.startUtc, c.endUtc, now), w0 - 8, y + 26);
    }
}

void draw(bool full) {
    if (full) s_lastSig = -1;

    // Tick the signature once a minute so "starts in" stays fresh, and on size
    // changes.
    int sig = (int)Contests::upcoming().size() * 131
            + (int)(time(nullptr) / 60)
            + s_scroll * 7;
    if (sig == s_lastSig && !full) return;
    s_lastSig = sig;

    drawSubHeader();
    drawScrollButtons();
    drawList();
}

void touch(int x, int y) {
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
