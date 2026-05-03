#include "ui_internal.h"

#include <vector>
#include <algorithm>
#include <string.h>
#include "../dx_cluster.h"
#include "../config.h"
#include "../callsign.h"

namespace Ui { namespace ScreenDx {

// Filter pills: tap the value to cycle forward, tap "<" / ">" arrows to step.

static const char* kBands[] = {
    "any", "160m", "80m", "60m", "40m", "30m",
    "20m", "17m", "15m", "12m", "10m", "6m", "2m"
};
static constexpr int kBandCount = sizeof(kBands) / sizeof(kBands[0]);

static const char* kModes[] = {
    "any", "CW", "SSB", "FT8", "FT4", "RTTY", "DIGI"
};
static constexpr int kModeCount = sizeof(kModes) / sizeof(kModes[0]);

// Continent filter has just two states: show everything, or show only spots
// whose spotter is on the same continent as the user's callsign. The pill
// label shows the detected continent in brackets when "my" is active.
static const char* kConts[] = { "any", "my" };
static constexpr int kContCount = sizeof(kConts) / sizeof(kConts[0]);

static int s_scroll = 0;
static const int kRowH = 18;
static int s_visibleRows = 0;

static int s_lastDrawnSig = -1;

// Hit rects.
static Rect s_bandPrev{0,0,0,0}, s_bandPill{0,0,0,0}, s_bandNext{0,0,0,0};
static Rect s_modePrev{0,0,0,0}, s_modePill{0,0,0,0}, s_modeNext{0,0,0,0};
static Rect s_contPrev{0,0,0,0}, s_contPill{0,0,0,0}, s_contNext{0,0,0,0};
static Rect s_btnUp{0,0,0,0}, s_btnDown{0,0,0,0};

// Filter rows live in the sub-header area + a few pixels of body, leaving the
// rest of the body for the spot list.
static int filtersY()    { return CONTENT_Y + 4; }
static int filtersH()    { return 50; }
static int listY()       { return filtersY() + filtersH() + 4; }
static int listBottom()  { return SCREEN_H - 4; }
static int listRightCol(){ return SCREEN_W - 30; }

static int indexOf(const char* const arr[], int n, const String& v) {
    for (int i = 0; i < n; i++) if (v == arr[i]) return i;
    return 0;
}

static bool matchesFilter(const DxSpot& sp) {
    auto& cfg = Config::get();
    if (cfg.filterBand != "any" && cfg.filterBand != sp.band) return false;
    if (cfg.filterMode != "any") {
        String want = cfg.filterMode;
        String have = sp.mode;
        want.toUpperCase();
        have.toUpperCase();
        if (want == "DIGI") {
            if (have != "FT8" && have != "FT4" && have != "RTTY" &&
                have != "PSK" && have != "JT65" && have != "JT9" &&
                have != "DIGI") return false;
        } else if (want != have) {
            return false;
        }
    }
    if (cfg.filterCont == "my") {
        const char* mine = Callsign::continent(cfg.myCallsign);
        const char* his  = Callsign::continent(sp.spotter);
        // If we can't classify the user's own callsign the filter is a no-op
        // (otherwise we'd hide everything). If we can't classify the spotter
        // we drop the spot - safer than letting unknown prefixes leak in.
        if (!mine) return true;
        if (!his || strcmp(mine, his) != 0) return false;
    }
    return true;
}

static int countMatches(const std::deque<DxSpot>& spots) {
    int n = 0;
    for (auto& s : spots) if (matchesFilter(s)) n++;
    return n;
}

static void drawFilterRow(int y, const char* label, const String& value,
                          Rect& outPrev, Rect& outPill, Rect& outNext) {
    auto& d = M5.Display;
    d.setFont(&fonts::Font0);
    d.setTextColor(COL_MUTED, COL_PANEL);
    d.setTextDatum(middle_left);
    d.drawString(label, 8, y + 11);

    int prevX = 50,  prevW = 22;
    int pillX = prevX + prevW + 4;
    int pillW = 80;
    int nextX = pillX + pillW + 4;
    int nextW = 22;
    int h = 22;

    outPrev = drawButton(prevX, y, prevW, h, "<", COL_CARD, COL_FG);
    // Pill (filled, accent-colored if non-default).
    bool active = (value != "any");
    uint16_t bg = active ? COL_ACCENT_D : COL_CARD;
    uint16_t fg = COL_FG;
    d.fillRoundRect(pillX, y, pillW, h, 11, bg);
    d.drawRoundRect(pillX, y, pillW, h, 11, active ? COL_ACCENT : COL_BORDER);
    d.setTextColor(fg, bg);
    d.setTextDatum(middle_center);
    d.setFont(&fonts::Font2);
    d.drawString(value, pillX + pillW / 2, y + h / 2);
    outPill = { pillX, y, pillW, h };

    outNext = drawButton(nextX, y, nextW, h, ">", COL_CARD, COL_FG);

    // Match count summary on the right of the band row only.
    if (label[0] == 'B') {
        int matches = countMatches(DxCluster::spots());
        String tag = String(matches) + " / " + String((int)DxCluster::spotCount());
        d.setFont(&fonts::Font2);
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setTextDatum(middle_right);
        d.drawString(tag, SCREEN_W - 8, y + h / 2);
    }
}

// Compact tap-to-toggle pill for filters with only two states (no prev/next
// arrows). Returns the hit rect via outRect.
static void drawTogglePill(int x, int y, int w, int h, const char* label,
                           const String& value, bool active, Rect& outRect) {
    auto& d = M5.Display;
    d.setFont(&fonts::Font0);
    d.setTextColor(COL_MUTED, COL_PANEL);
    d.setTextDatum(middle_left);
    d.drawString(label, x, y + h / 2);

    int pillX = x + 24;
    int pillW = w - 24;
    uint16_t bg = active ? COL_ACCENT_D : COL_CARD;
    d.fillRoundRect(pillX, y, pillW, h, 11, bg);
    d.drawRoundRect(pillX, y, pillW, h, 11, active ? COL_ACCENT : COL_BORDER);
    d.setTextColor(COL_FG, bg);
    d.setTextDatum(middle_center);
    d.setFont(&fonts::Font2);
    d.drawString(value, pillX + pillW / 2, y + h / 2);
    outRect = { pillX, y, pillW, h };
}

static void drawFilters() {
    auto& cfg = Config::get();
    auto& d = M5.Display;
    d.fillRect(0, filtersY() - 2, SCREEN_W, filtersH() + 4, COL_PANEL);
    drawFilterRow(filtersY(),      "BAND", cfg.filterBand,
                  s_bandPrev, s_bandPill, s_bandNext);
    drawFilterRow(filtersY() + 26, "MODE", cfg.filterMode,
                  s_modePrev, s_modePill, s_modeNext);

    // Continent toggle: sits on the right side of the MODE row so we don't
    // need a third row. When "my" is active we show "my [XX]" so the user
    // can see which continent we inferred from their own callsign.
    String contLabel = cfg.filterCont;
    bool contActive = (contLabel != "any");
    if (contLabel == "my") {
        const char* mine = Callsign::continent(cfg.myCallsign);
        contLabel = String("my [") + (mine ? mine : "??") + "]";
    }
    // Hide-handles for the prev/next arrows are unused for the DE pill;
    // collapse them to zero-size so they never match a touch.
    s_contPrev = {0,0,0,0};
    s_contNext = {0,0,0,0};
    drawTogglePill(190, filtersY() + 26, 122, 22,
                   "DE", contLabel, contActive, s_contPill);
}

static void drawScrollControls() {
    int x = listRightCol() + 4;
    int wBtn = 22;
    int hBtn = 26;
    int yTop = listY() + 2;
    int yBot = listBottom() - hBtn - 2;
    s_btnUp   = drawButton(x, yTop, wBtn, hBtn, "^", COL_CARD, COL_FG);
    s_btnDown = drawButton(x, yBot, wBtn, hBtn, "v", COL_CARD, COL_FG);
}

static void drawList(const std::deque<DxSpot>& spots) {
    auto& d = M5.Display;
    int x0 = 0, y0 = listY();
    int w0 = listRightCol(), h0 = listBottom() - listY();
    d.fillRect(x0, y0, w0, h0, COL_PANEL);

    s_visibleRows = h0 / kRowH;

    std::vector<const DxSpot*> view;
    view.reserve(spots.size());
    for (auto& s : spots) if (matchesFilter(s)) view.push_back(&s);

    if (view.empty()) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        String msg = spots.empty()
            ? DxCluster::status()
            : String("No spots match");
        d.drawString(msg, SCREEN_W / 2, y0 + h0 / 2);
        return;
    }

    int hy = y0;
    d.fillRect(0, hy, w0, kRowH, COL_CARD);
    d.setTextColor(COL_MUTED, COL_CARD);
    d.setFont(&fonts::Font0);
    d.setTextDatum(top_left);
    d.drawString("BAND", 8,   hy + 5);
    d.drawString("FREQ", 50,  hy + 5);
    d.drawString("DX",   115, hy + 5);
    d.drawString("MODE", 200, hy + 5);
    d.drawString("AGE",  245, hy + 5);

    int rows = s_visibleRows - 1;
    if (rows < 1) rows = 1;
    int start = s_scroll;
    if (start < 0) start = 0;
    if (start > (int)view.size() - 1) start = (int)view.size() - 1;
    if (start < 0) start = 0;

    uint32_t now = millis();
    for (int i = 0; i < rows; i++) {
        int idx = start + i;
        if (idx >= (int)view.size()) break;
        const DxSpot& sp = *view[idx];
        int y = hy + kRowH + i * kRowH;
        uint16_t bg = (i & 1) ? COL_BG : COL_PANEL;
        d.fillRect(0, y, w0, kRowH, bg);

        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        d.setTextColor(COL_ACCENT, bg);
        d.drawString(sp.band, 8, y + kRowH / 2);

        d.setTextColor(COL_FG, bg);
        char freqBuf[16];
        snprintf(freqBuf, sizeof(freqBuf), "%.1f", sp.freqKHz);
        d.drawString(freqBuf, 50, y + kRowH / 2);

        d.drawString(sp.dx.substring(0, 11), 115, y + kRowH / 2);

        d.setTextColor(COL_WARN, bg);
        d.drawString(sp.mode.substring(0, 4), 200, y + kRowH / 2);

        uint32_t ageS = (now - sp.rxMillis) / 1000;
        char ageBuf[8];
        if (ageS < 60)         snprintf(ageBuf, sizeof(ageBuf), "%lus", (unsigned long)ageS);
        else if (ageS < 3600)  snprintf(ageBuf, sizeof(ageBuf), "%lum", (unsigned long)(ageS / 60));
        else                   snprintf(ageBuf, sizeof(ageBuf), "%luh", (unsigned long)(ageS / 3600));
        d.setTextColor(COL_DIM, bg);
        d.drawString(ageBuf, 245, y + kRowH / 2);
    }
}

void draw(bool full) {
    auto& spots = DxCluster::spots();
    auto& cfg = Config::get();

    if (full) {
        // The top header and content background are drawn by ui.cpp.
        drawFilters();
        drawScrollControls();
        s_lastDrawnSig = -1;
    }

    int sig = (int)spots.size() * 31 + countMatches(spots);
    sig = sig * 31 + s_scroll;
    for (auto c : cfg.filterBand) sig = sig * 31 + c;
    for (auto c : cfg.filterMode) sig = sig * 31 + c;
    for (auto c : cfg.filterCont) sig = sig * 31 + c;

    if (sig == s_lastDrawnSig) return;
    s_lastDrawnSig = sig;

    drawFilters();          // refresh match count + selected pill background
    drawList(spots);
    drawScrollControls();   // covered by clearContent on full only
}

static void cycle(String& cur, const char* const arr[], int n, int delta) {
    int i = indexOf(arr, n, cur);
    i = (i + delta + n) % n;
    cur = arr[i];
}

void touch(int x, int y) {
    auto& cfg = Config::get();

    if (s_bandPrev.contains(x, y)) { cycle(cfg.filterBand, kBands, kBandCount, -1); }
    else if (s_bandNext.contains(x, y) || s_bandPill.contains(x, y)) { cycle(cfg.filterBand, kBands, kBandCount, +1); }
    else if (s_modePrev.contains(x, y)) { cycle(cfg.filterMode, kModes, kModeCount, -1); }
    else if (s_modeNext.contains(x, y) || s_modePill.contains(x, y)) { cycle(cfg.filterMode, kModes, kModeCount, +1); }
    else if (s_contPrev.contains(x, y)) { cycle(cfg.filterCont, kConts, kContCount, -1); }
    else if (s_contNext.contains(x, y) || s_contPill.contains(x, y)) { cycle(cfg.filterCont, kConts, kContCount, +1); }
    else {
        // Scroll buttons.
        int rows = s_visibleRows - 1;
        if (rows < 1) rows = 1;
        if (s_btnUp.contains(x, y)) {
            s_scroll = std::max(0, s_scroll - rows);
            s_lastDrawnSig = -1;
        } else if (s_btnDown.contains(x, y)) {
            s_scroll = s_scroll + rows;
            s_lastDrawnSig = -1;
        }
        return;
    }

    // Filter changed - persist and trigger redraw.
    Config::save();
    s_scroll = 0;
    s_lastDrawnSig = -1;
}

}}
