#include "ui_internal.h"

#include <math.h>
#include <time.h>
#include "../sun.h"
#include "../config.h"
#include "../geo.h"
#include "../beacons.h"

namespace Ui { namespace ScreenGrayline {

// Rectangular world map with day/night shading + grayline + sun position.
//
// Layout inside the content area:
//   y = CONTENT_Y     ..  CONTENT_Y + 22       info strip (sun lat/lon, UTC)
//   y = CONTENT_Y+22  ..  SCREEN_H              equirectangular map (320 wide)
//
// The map uses one pixel per 1.125° longitude (320 px / 360°) and roughly
// one pixel per 1° latitude (180 px / 180°).

static const int kInfoH = 22;
static int s_mapY0;
static int s_mapH;

static int s_lastTickMin = -1;   // recompute every minute (sun moves slowly)

static float deg2rad(float d) { return d * (float)M_PI / 180.0f; }

static int lonToX(float lon) {
    while (lon < -180.0f) lon += 360.0f;
    while (lon >  180.0f) lon -= 360.0f;
    return (int)((lon + 180.0f) * SCREEN_W / 360.0f + 0.5f);
}
static int latToY(float lat) {
    if (lat >  90.0f) lat =  90.0f;
    if (lat < -90.0f) lat = -90.0f;
    return s_mapY0 + (int)((90.0f - lat) * s_mapH / 180.0f + 0.5f);
}

static void drawInfo(time_t now) {
    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y, SCREEN_W, kInfoH, COL_PANEL);

    float decl = Sun::declinationDeg(now);
    float subLon = Sun::subSolarLonDeg(now);

    char buf[64];
    if (Sun::ready()) {
        snprintf(buf, sizeof(buf), "Sun:  %4.1f%c %s   %5.1f%c %s",
                 fabsf(decl), (char)0xB0, decl >= 0 ? "N" : "S",
                 fabsf(subLon), (char)0xB0, subLon >= 0 ? "E" : "W");
    } else {
        snprintf(buf, sizeof(buf), "Waiting for time sync...");
    }
    d.setFont(&fonts::Font2);
    d.setTextDatum(middle_left);
    d.setTextColor(COL_FG, COL_PANEL);
    d.drawString(buf, 8, CONTENT_Y + kInfoH / 2);

    // Right side: clock seconds.
    if (Sun::ready()) {
        struct tm tmv;
        gmtime_r(&now, &tmv);
        char clk[16];
        snprintf(clk, sizeof(clk), "%02d:%02d:%02dZ",
                 tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        d.setTextDatum(middle_right);
        d.setTextColor(COL_DIM, COL_PANEL);
        d.drawString(clk, SCREEN_W - 8, CONTENT_Y + kInfoH / 2);
    }
}

static void drawMap(time_t now) {
    auto& d = M5.Display;

    // Cache trig once per redraw.
    float decl   = Sun::declinationDeg(now);
    float subLon = Sun::subSolarLonDeg(now);
    float declR  = deg2rad(decl);
    float sinDecl = sinf(declR);
    float cosDecl = cosf(declR);

    bool poleIsDay = (decl > 0.0f);  // North pole is day if Northern summer

    // Fill day/night per column using the terminator latitude. For each x we
    // solve sin(d) sin(l) + cos(d) cos(l) cos(lon - subLon) = 0.
    for (int x = 0; x < SCREEN_W; x++) {
        float lon = -180.0f + x * 360.0f / SCREEN_W;
        float lonDiffR = deg2rad(lon - subLon);
        float cosDiff = cosf(lonDiffR);

        float termLat;
        if (fabsf(sinDecl) < 0.001f) {
            // Equinox: terminator is two meridians at ±90° from sub-solar lon.
            // Approximate by using a vertical line; choose lat such that the
            // entire column is on one side.
            float diffNorm = lon - subLon;
            while (diffNorm >  180.0f) diffNorm -= 360.0f;
            while (diffNorm < -180.0f) diffNorm += 360.0f;
            bool dayCol = (fabsf(diffNorm) < 90.0f);
            d.drawFastVLine(x, s_mapY0, s_mapH,
                            dayCol ? COL_PANEL : COL_BG);
            continue;
        } else {
            termLat = atanf(-cosDecl * cosDiff / sinDecl);
        }
        float termLatDeg = termLat * 180.0f / (float)M_PI;
        int yTerm = latToY(termLatDeg);
        if (yTerm < s_mapY0)            yTerm = s_mapY0;
        if (yTerm > s_mapY0 + s_mapH)   yTerm = s_mapY0 + s_mapH;

        if (poleIsDay) {
            d.drawFastVLine(x, s_mapY0, yTerm - s_mapY0,         COL_PANEL);
            d.drawFastVLine(x, yTerm,   s_mapY0 + s_mapH - yTerm, COL_BG);
        } else {
            d.drawFastVLine(x, s_mapY0, yTerm - s_mapY0,         COL_BG);
            d.drawFastVLine(x, yTerm,   s_mapY0 + s_mapH - yTerm, COL_PANEL);
        }
    }

    // Latitude grid (subtle dotted lines).
    static const int kLats[] = { -60, -30, 0, 30, 60 };
    for (int la : kLats) {
        int y = latToY((float)la);
        for (int xx = 0; xx < SCREEN_W; xx += 4) {
            d.drawPixel(xx, y, COL_BORDER);
        }
    }
    // Longitude grid (every 60°).
    static const int kLons[] = { -120, -60, 0, 60, 120 };
    for (int lo : kLons) {
        int x = lonToX((float)lo);
        for (int yy = s_mapY0; yy < s_mapY0 + s_mapH; yy += 4) {
            d.drawPixel(x, yy, COL_BORDER);
        }
    }

    // Twilight band: 12° wide, centred on the terminator. We sample a few
    // longitudes and draw the latitudes where altitude is in (-12°, +12°).
    // For each longitude we compute term latitude and ±band edges.
    for (int x = 0; x < SCREEN_W; x++) {
        float lon = -180.0f + x * 360.0f / SCREEN_W;
        float lonDiffR = deg2rad(lon - subLon);
        float cosDiff = cosf(lonDiffR);

        if (fabsf(sinDecl) < 0.001f) continue;
        float termLat = atanf(-cosDecl * cosDiff / sinDecl);
        float termLatDeg = termLat * 180.0f / (float)M_PI;
        int yTerm = latToY(termLatDeg);
        if (yTerm >= s_mapY0 && yTerm < s_mapY0 + s_mapH) {
            d.drawPixel(x, yTerm, COL_ACCENT);
        }
    }

    // Sun symbol at the sub-solar point.
    int sunX = lonToX(subLon);
    int sunY = latToY(decl);
    d.fillCircle(sunX, sunY, 3, COL_WARN);
    d.drawCircle(sunX, sunY, 6, COL_WARN);

    // QTH dot if grid set.
    auto& cfg = Config::get();
    if (cfg.myGrid.length() >= 4) {
        float la, lo;
        if (Geo::gridToLatLon(cfg.myGrid, la, lo)) {
            int qx = lonToX(lo);
            int qy = latToY(la);
            d.fillCircle(qx, qy, 3, COL_OK);
            d.drawCircle(qx, qy, 5, COL_OK);
        }
    }

    // NCDXF beacons as small dots so the map feels populated.
    for (int i = 0; i < Beacons::kStationCount; i++) {
        const auto& s = Beacons::kStations[i];
        int bx = lonToX(s.lon);
        int by = latToY(s.lat);
        d.drawPixel(bx,     by,     COL_FG);
        d.drawPixel(bx + 1, by,     COL_FG);
        d.drawPixel(bx,     by + 1, COL_FG);
    }
}

void draw(bool full) {
    s_mapY0 = CONTENT_Y + kInfoH;
    s_mapH  = SCREEN_H - s_mapY0;

    time_t now = time(nullptr);
    struct tm tmv;
    gmtime_r(&now, &tmv);
    int tickKey = tmv.tm_min;          // recompute once per minute

    if (full) s_lastTickMin = -1;
    if (tickKey == s_lastTickMin && !full) {
        // Just refresh the seconds line.
        drawInfo(now);
        return;
    }
    s_lastTickMin = tickKey;

    drawInfo(now);
    drawMap(now);
}

void touch(int /*x*/, int /*y*/) { /* no interaction yet */ }

}}
