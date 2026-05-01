#include "ui_internal.h"

#include <math.h>
#include "../config.h"
#include "../geo.h"

namespace Ui { namespace ScreenBearing {

// Layout: a "From / To" card at the top with two tappable rows, then a
// big "result" card with bearing degrees, compass label, and distance.

static String s_target = "";       // last entered target grid; cleared on demand
static String s_lastSig;
static Rect s_fromRow { 0, 0, 0, 0 };
static Rect s_toRow   { 0, 0, 0, 0 };

static String fmtKm(float km) {
    char b[24];
    if (km < 10.0f)  snprintf(b, sizeof(b), "%.1f km", km);
    else             snprintf(b, sizeof(b), "%.0f km", km);
    return String(b);
}

static String fmtMi(float km) {
    char b[24];
    float mi = km * 0.62137f;
    if (mi < 10.0f)  snprintf(b, sizeof(b), "%.1f mi", mi);
    else             snprintf(b, sizeof(b), "%.0f mi", mi);
    return String(b);
}

static void drawInputRow(int y, const char* label, const String& value, Rect& outRect) {
    auto& d = M5.Display;
    int x = 6, w = SCREEN_W - 12, h = 30;
    drawCard(x, y, w, h);
    d.setFont(&fonts::Font0);
    d.setTextColor(COL_MUTED, COL_CARD);
    d.setTextDatum(middle_left);
    d.drawString(label, x + 10, y + 10);

    d.setFont(&fonts::Font4);
    d.setTextColor(value.length() ? COL_FG : COL_DIM, COL_CARD);
    d.setTextDatum(middle_right);
    d.drawString(value.length() ? value : String("(tap to set)"),
                 x + w - 28, y + h / 2);

    d.setFont(&fonts::Font2);
    d.setTextColor(COL_ACCENT, COL_CARD);
    d.setTextDatum(middle_right);
    d.drawString(">", x + w - 8, y + h / 2);

    outRect = { x, y, w, h };
}

static bool validGrid(const String& g) {
    if (g.length() != 4 && g.length() != 6) return false;
    float la, lo;
    return Geo::gridToLatLon(g, la, lo);
}

static void drawResult() {
    auto& d = M5.Display;
    int x = 6, y = CONTENT_Y + 4 + 30 + 4 + 30 + 6;  // below the two input rows
    int w = SCREEN_W - 12;
    int h = SCREEN_H - 4 - y;
    drawCard(x, y, w, h);
    d.fillRoundRect(x, y, w, h, 6, COL_CARD);
    d.drawRoundRect(x, y, w, h, 6, COL_BORDER);

    auto& cfg = Config::get();
    String from = cfg.myGrid;
    String to   = s_target;

    if (!validGrid(from)) {
        d.setFont(&fonts::Font2);
        d.setTextColor(COL_WARN, COL_CARD);
        d.setTextDatum(middle_center);
        d.drawString("Set your QTH grid in Settings -> My Grid",
                     x + w / 2, y + h / 2);
        return;
    }
    if (!validGrid(to)) {
        d.setFont(&fonts::Font2);
        d.setTextColor(COL_DIM, COL_CARD);
        d.setTextDatum(middle_center);
        d.drawString("Tap \"To\" above to enter a target grid",
                     x + w / 2, y + h / 2);
        return;
    }

    float la1, lo1, la2, lo2;
    Geo::gridToLatLon(from, la1, lo1);
    Geo::gridToLatLon(to,   la2, lo2);
    float bearing = Geo::bearingDeg(la1, lo1, la2, lo2);
    float dist    = Geo::distanceKm(la1, lo1, la2, lo2);
    float longPath = 360.0f - bearing;
    if (longPath >= 360.0f) longPath -= 360.0f;
    float longDist = 40075.017f - dist;

    // Big bearing degrees + compass label.
    char buf[32];
    snprintf(buf, sizeof(buf), "%03d", (int)(bearing + 0.5f));
    d.setFont(&fonts::Font7);
    d.setTextColor(COL_ACCENT, COL_CARD);
    d.setTextDatum(top_center);
    d.drawString(buf, x + w / 2 - 36, y + 6);

    d.setFont(&fonts::Font4);
    d.setTextColor(COL_FG, COL_CARD);
    d.setTextDatum(top_left);
    d.drawString(String((char)0xB0) + " " + Geo::compassLabel(bearing),
                 x + w / 2 + 50, y + 16);

    // Distance line.
    d.setFont(&fonts::Font2);
    d.setTextColor(COL_DIM, COL_CARD);
    d.setTextDatum(middle_left);
    d.drawString("short path",  x + 14, y + h - 38);
    d.setTextColor(COL_FG, COL_CARD);
    d.setTextDatum(middle_right);
    d.drawString(fmtKm(dist) + " / " + fmtMi(dist),
                 x + w - 14, y + h - 38);

    // Long path heading.
    snprintf(buf, sizeof(buf), "%03d%c %s", (int)(longPath + 0.5f), (char)0xB0,
             Geo::compassLabel(longPath));
    d.setTextColor(COL_DIM, COL_CARD);
    d.setTextDatum(middle_left);
    d.drawString("long path",   x + 14, y + h - 18);
    d.setTextColor(COL_FG, COL_CARD);
    d.setTextDatum(middle_right);
    d.drawString(String(buf) + "  " + fmtKm(longDist),
                 x + w - 14, y + h - 18);
}

void draw(bool full) {
    auto& cfg = Config::get();
    String sig = cfg.myGrid + "|" + s_target;
    if (full) s_lastSig = "";
    if (sig == s_lastSig) return;
    s_lastSig = sig;

    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, COL_PANEL);

    int y1 = CONTENT_Y + 4;
    int y2 = y1 + 34;
    drawInputRow(y1, "FROM (your QTH)",
                 cfg.myGrid.length() ? cfg.myGrid : String(""), s_fromRow);
    drawInputRow(y2, "TO",
                 s_target.length() ? s_target : String(""), s_toRow);
    drawResult();
}

void touch(int x, int y) {
    if (s_fromRow.contains(x, y)) {
        auto& cfg = Config::get();
        String v = cfg.myGrid;
        if (Ui::editString("Your QTH grid (e.g. FN30as)", &v, false, 8)) {
            v.trim();
            cfg.myGrid = v;
            Config::save();
        }
        Ui::requestFullRedraw();
        return;
    }
    if (s_toRow.contains(x, y)) {
        String v = s_target;
        if (Ui::editString("Target grid (e.g. JN58td)", &v, false, 8)) {
            v.trim();
            s_target = v;
        }
        Ui::requestFullRedraw();
        return;
    }
}

}}
