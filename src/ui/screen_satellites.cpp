#include "ui_internal.h"
#include "icons.h"

#include <algorithm>
#include <time.h>
#include "../satellites.h"

namespace Ui { namespace ScreenSatellites {

static int  s_scroll = 0;
static int  s_lastSig = -1;
static Rect s_btnRefresh { 0, 0, 0, 0 };
static Rect s_btnUp      { 0, 0, 0, 0 };
static Rect s_btnDown    { 0, 0, 0, 0 };
static const int kRowH = 36;
static int  s_visibleRows = 0;

static void drawSubHeader() {
    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y, SCREEN_W, SUBHEADER_H, COL_PANEL);

    d.setFont(&fonts::Font2);
    d.setTextColor(COL_DIM, COL_PANEL);
    d.setTextDatum(middle_left);
    String left = String((unsigned)Satellites::tleCount()) + " sats - " + Satellites::status();
    if (left.length() > 28) left = left.substring(0, 28);
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
    int wBtn = 22, hBtn = 26;
    int yTop = CONTENT_BODY_Y + 4;
    int yBot = SCREEN_H - hBtn - 4;
    s_btnUp   = drawButton(x, yTop, wBtn, hBtn, "^", COL_CARD, COL_FG);
    s_btnDown = drawButton(x, yBot, wBtn, hBtn, "v", COL_CARD, COL_FG);
}

static String fmtTime(time_t t) {
    struct tm tmv;
    gmtime_r(&t, &tmv);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", tmv.tm_hour, tmv.tm_min);
    return String(buf);
}

static String fmtDay(time_t t) {
    struct tm tmv;
    gmtime_r(&t, &tmv);
    static const char* dn[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    return String(dn[tmv.tm_wday]);
}

static String fmtRelStart(time_t t, time_t now) {
    long delta = (long)(t - now);
    if (delta < 0) return String("now");
    if (delta < 3600)  return String("in ") + (delta / 60) + "m";
    if (delta < 86400) {
        char b[16];
        snprintf(b, sizeof(b), "in %ldh%02ldm", delta / 3600, (delta % 3600) / 60);
        return String(b);
    }
    return String("in ") + (delta / 86400) + "d";
}

static const char* compass16(float az) {
    static const char* c[] = {
        "N","NNE","NE","ENE","E","ESE","SE","SSE",
        "S","SSW","SW","WSW","W","WNW","NW","NNW"
    };
    int i = ((int)((az + 11.25f) / 22.5f)) & 0xF;
    return c[i];
}

static void drawList() {
    auto& d = M5.Display;
    int x0 = 0, y0 = CONTENT_BODY_Y;
    int w0 = listRightCol(), h0 = SCREEN_H - CONTENT_BODY_Y;
    d.fillRect(x0, y0, w0, h0, COL_PANEL);

    s_visibleRows = h0 / kRowH;
    auto& list = Satellites::passes();

    if (list.empty()) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        String msg;
        if (!Satellites::ready())            msg = "computing...";
        else if (Satellites::tleCount() == 0) msg = "no TLEs - check WiFi";
        else                                  msg = "no upcoming passes";
        d.drawString(msg, w0 / 2, y0 + h0 / 2);
        return;
    }

    time_t now = time(nullptr);
    int start = s_scroll;
    if (start < 0) start = 0;
    if (start > (int)list.size() - 1) start = (int)list.size() - 1;

    for (int i = 0; i < s_visibleRows; i++) {
        int idx = start + i;
        if (idx >= (int)list.size()) break;
        const SatPass& p = list[idx];
        int y = y0 + i * kRowH;
        uint16_t bg = (i & 1) ? COL_BG : COL_PANEL;
        d.fillRect(0, y, w0, kRowH, bg);

        // Top row: name + AOS day/time + relative start
        int topMid = y + 11;
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_ACCENT, bg);
        d.drawString(p.name, 6, topMid);

        d.setTextColor(COL_FG, bg);
        String when = fmtDay(p.aos) + " " + fmtTime(p.aos) + "Z";
        d.drawString(when, 100, topMid);

        d.setTextColor(COL_DIM, bg);
        d.setTextDatum(middle_right);
        d.drawString(fmtRelStart(p.aos, now), w0 - 6, topMid);

        // Bottom row: max elevation, AOS->LOS azimuth, duration
        int botMid = y + 26;
        d.setFont(&fonts::Font0);
        d.setTextDatum(middle_left);
        char el[16];
        snprintf(el, sizeof(el), "max %d%c", (int)(p.maxEl + 0.5f), (char)0xB0);
        uint16_t elColor = (p.maxEl >= 30.0f) ? COL_OK : (p.maxEl >= 15.0f ? COL_WARN : COL_DIM);
        d.setTextColor(elColor, bg);
        d.drawString(el, 6, botMid);

        d.setTextColor(COL_DIM, bg);
        char path[24];
        snprintf(path, sizeof(path), "%s -> %s",
                 compass16(p.aosAz), compass16(p.losAz));
        d.drawString(path, 70, botMid);

        long dur = (long)(p.los - p.aos);
        char db[16];
        if (dur < 60) snprintf(db, sizeof(db), "%lds", dur);
        else          snprintf(db, sizeof(db), "%ldm%02lds", dur / 60, dur % 60);
        d.setTextDatum(middle_right);
        d.drawString(db, w0 - 6, botMid);
    }
}

void draw(bool full) {
    if (full) s_lastSig = -1;

    auto& list = Satellites::passes();
    int sig = (int)list.size() * 131
            + s_scroll * 7
            + (int)(Satellites::lastFetchAgeSeconds() / 30)
            + (int)(time(nullptr) / 60);  // for relative-time refresh
    if (sig == s_lastSig && !full) return;
    s_lastSig = sig;

    drawSubHeader();
    drawScrollButtons();
    drawList();
}

void touch(int x, int y) {
    if (s_btnRefresh.contains(x, y)) {
        Satellites::requestRefresh();
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
