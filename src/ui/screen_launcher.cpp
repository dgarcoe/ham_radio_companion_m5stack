#include "ui_internal.h"
#include "icons.h"

#include <time.h>
#include <vector>
#include "../config.h"
#include "../wifi_manager.h"
#include "../dx_cluster.h"
#include "../propagation.h"
#include "../alerts.h"
#include "../beacons.h"
#include "../pota.h"
#include "../noaa.h"
#include "../contests.h"
#include "../sun.h"
#include "../satellites.h"
#include "../sota.h"

namespace Ui { namespace ScreenLauncher {

// Top banner (clock + callsign + date + battery) followed by a 4x3 tile grid.

static const int kBannerH = 56;

static const int kGridY = kBannerH + 4;
static const int kGridH = SCREEN_H - kGridY - 4;
static const int kCols = 4;
static const int kRows = 3;
static const int kGridGap = 4;

// The battery indicator lives in the top-right of the banner; reserve a bit
// of horizontal space so the callsign/date don't run into it.
static const int kBatX = SCREEN_W - 16;     // body left edge
static const int kBatW = 10;
static const int kBatH = 30;
static const int kBatY = (kBannerH - kBatH) / 2;
static const int kBatRightMargin = 22;      // text must end before this

struct Tile {
    Screen target;
    const char* label;
    Icons::Icon icon;
    String (*statusFn)();
};

static String dxStatus()    {
    if (DxCluster::isConnected()) return String((unsigned)DxCluster::spotCount()) + " spots";
    return String("offline");
}
static String propStatus()  {
    auto& p = Propagation::data();
    if (!p.valid) return "no data";
    return String("SFI ") + p.solarFlux;
}
static String alertsStatus() {
    int active = 0;
    for (auto& r : Config::get().alerts) if (r.enabled) active++;
    int hits = (int)Alerts::history().size();
    // Compact form so it always fits a 4-col launcher tile.
    if (hits == 0) return String(active) + " active";
    return String(active) + " on, " + String(hits) + "h";
}
static String beaconsStatus() {
    int idx = Beacons::currentStationIndex(0);
    if (idx < 0) return "5 bands";
    return Beacons::kStations[idx].call;
}
static String potaStatus() {
    if (Pota::lastFetchAgeSeconds() == UINT32_MAX) return Pota::status();
    return String((unsigned)Pota::spotCount()) + " parks";
}
static String bearingStatus() {
    auto& cfg = Config::get();
    return cfg.myGrid.length() ? cfg.myGrid : String("set QTH");
}
static String noaaStatus() {
    if (Noaa::lastFetchAgeSeconds() == UINT32_MAX) return Noaa::status();
    return String((unsigned)Noaa::count()) + " alerts";
}
static String contestsStatus() {
    if (!Contests::ready()) return "no time";
    auto& list = Contests::upcoming();
    if (list.empty()) return "none";
    time_t now = time(nullptr);
    int running = 0;
    for (auto& c : list) if (now >= c.startUtc && now < c.endUtc) running++;
    if (running > 0) return String(running) + " running";
    long delta = (long)(list.front().startUtc - now);
    if (delta < 86400) return "next " + String(delta / 3600) + "h";
    return "next " + String(delta / 86400) + "d";
}
static String graylineStatus() {
    if (!Sun::ready()) return "no time";
    float decl = Sun::declinationDeg(time(nullptr));
    char b[12];
    snprintf(b, sizeof(b), "%+0.1f%c lat", decl, (char)0xB0);
    return String(b);
}
static String satellitesStatus() {
    auto& list = Satellites::passes();
    if (list.empty()) return Satellites::status();
    time_t now = time(nullptr);
    long delta = (long)(list.front().aos - now);
    if (delta < 0) return "now!";
    if (delta < 3600)  return "in " + String(delta / 60) + "m";
    if (delta < 86400) return "in " + String(delta / 3600) + "h";
    return "in " + String(delta / 86400) + "d";
}
static String sotaStatus() {
    if (Sota::lastFetchAgeSeconds() == UINT32_MAX) return Sota::status();
    return String((unsigned)Sota::spotCount()) + " spots";
}
static String settingsStatus(){ return Config::loadSource(); }

static const Tile kTiles[] = {
    { Screen::DxCluster,   "DX",         Icons::DxIcon,       dxStatus       },
    { Screen::Propagation, "Prop",       Icons::PropIcon,     propStatus     },
    { Screen::Beacons,     "Beacons",    Icons::BeaconIcon,   beaconsStatus  },
    { Screen::Pota,        "POTA",       Icons::PotaIcon,     potaStatus     },
    { Screen::Alerts,      "Alerts",     Icons::AlertsIcon,   alertsStatus   },
    { Screen::Bearing,     "Bearing",    Icons::BearingIcon,  bearingStatus  },
    { Screen::Noaa,        "Sp.Wx",      Icons::NoaaIcon,     noaaStatus     },
    { Screen::Contests,    "Contest",    Icons::ContestIcon,  contestsStatus },
    { Screen::Grayline,    "Grayline",   Icons::GraylineIcon, graylineStatus },
    { Screen::Satellites,  "Sats",       Icons::SatelliteIcon,satellitesStatus },
    { Screen::Sota,        "SOTA",       Icons::SotaIcon,     sotaStatus     },
    { Screen::Settings,    "Settings",   Icons::SettingsIcon, settingsStatus },
};
static constexpr int kTileCount = sizeof(kTiles) / sizeof(kTiles[0]);

struct TileHit { Rect r; int tileIdx; };
static std::vector<TileHit> s_tileHits;
static String s_tileLastStatus[kTileCount];   // indexed by tile index
static String s_lastClock;
static String s_lastCall;
static int    s_lastBatteryLevel = -1;
static int    s_lastBatteryCharging = -1;
static uint32_t s_lastHideMask = 0xFFFFFFFFu;

static String formatClock() {
    time_t now = time(nullptr);
    if (now < 1700000000) return "--:--:--";
    struct tm tmv;
    gmtime_r(&now, &tmv);
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    return String(buf);
}

static String formatDate() {
    time_t now = time(nullptr);
    if (now < 1700000000) return "----/--/-- UTC";
    struct tm tmv;
    gmtime_r(&now, &tmv);
    char buf[24];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d UTC",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
    return String(buf);
}

// Draw a vertical battery indicator. Body has a small cap on top to read as a
// battery silhouette; the fill rises from the bottom. Color codes the level
// (green / orange / red); charging is shown with a "+" suffix in the level
// readout below the bar.
static void drawBattery(bool full) {
    auto& d = M5.Display;
    int level = M5.Power.getBatteryLevel();
    if (level < 0)   level = 0;
    if (level > 100) level = 100;
    bool charging = M5.Power.isCharging();
    int chargingI = charging ? 1 : 0;

    if (!full && level == s_lastBatteryLevel && chargingI == s_lastBatteryCharging) return;
    s_lastBatteryLevel = level;
    s_lastBatteryCharging = chargingI;

    // Clear the entire battery area (cap + body + label below).
    d.fillRect(kBatX - 2, kBatY - 5, kBatW + 4, kBatH + 14, COL_BG);

    // Cap.
    int capW = 4, capH = 3;
    d.fillRect(kBatX + (kBatW - capW) / 2, kBatY - capH, capW, capH, COL_DIM);

    // Body outline.
    d.drawRect(kBatX, kBatY, kBatW, kBatH, COL_DIM);

    // Fill from the bottom proportional to level.
    int innerH = kBatH - 4;
    int fillH  = (innerH * level) / 100;
    int fillY  = kBatY + 2 + (innerH - fillH);
    uint16_t fc;
    if (charging)        fc = COL_OK;
    else if (level > 50) fc = COL_OK;
    else if (level > 20) fc = COL_WARN;
    else                 fc = COL_BAD;
    if (fillH > 0) d.fillRect(kBatX + 2, fillY, kBatW - 4, fillH, fc);

    // Tiny readout below the bar so the absolute number is also visible.
    char buf[8];
    snprintf(buf, sizeof(buf), charging ? "%d+" : "%d", level);
    d.setFont(&fonts::Font0);
    d.setTextColor(COL_DIM, COL_BG);
    d.setTextDatum(top_center);
    d.drawString(buf, kBatX + kBatW / 2, kBatY + kBatH + 1);
}

static void drawBanner(bool full) {
    auto& d = M5.Display;
    auto& cfg = Config::get();

    if (full) {
        d.fillRect(0, 0, SCREEN_W, kBannerH, COL_BG);
        d.drawFastHLine(0, kBannerH, SCREEN_W, COL_BORDER);
        s_lastClock = "";
        s_lastCall  = "";
    }

    // Big clock on the left. Clear area is wide enough for HH:MM:SS in Font7
    // — drawn flush to x=0 to use the few pixels of free space on the left.
    String clk = formatClock();
    if (clk != s_lastClock) {
        s_lastClock = clk;
        d.fillRect(0, 4, 214, kBannerH - 8, COL_BG);
        d.setFont(&fonts::Font7);
        d.setTextColor(COL_ACCENT, COL_BG);
        d.setTextDatum(top_left);
        d.drawString(clk, 0, 4);
    }

    // Callsign + date pushed right to leave room for the wider clock.
    int textRight = SCREEN_W - kBatRightMargin;
    int textLeft  = 216;
    if (cfg.myCallsign != s_lastCall || full) {
        s_lastCall = cfg.myCallsign;
        d.fillRect(textLeft, 4, textRight - textLeft, kBannerH - 8, COL_BG);
        d.setFont(&fonts::Font4);
        d.setTextColor(COL_FG, COL_BG);
        d.setTextDatum(top_right);
        d.drawString(cfg.myCallsign, textRight, 6);

        d.setFont(&fonts::Font0);
        d.setTextColor(COL_DIM, COL_BG);
        d.setTextDatum(top_right);
        d.drawString(formatDate(), textRight, 38);
    }

    drawBattery(full);
}

static void drawTileStatus(const Rect& r, const Tile& t, bool active) {
    auto& d = M5.Display;
    uint16_t bg = active ? COL_CARD_HI : COL_CARD;
    // Clear only the bottom strip where the status text lives.
    int sy = r.y + r.h - 14;
    int sh = 12;
    d.fillRect(r.x + 2, sy, r.w - 4, sh, bg);

    String status = t.statusFn ? t.statusFn() : String("");
    if (status.length()) {
        d.setFont(&fonts::Font0);
        d.setTextColor(COL_DIM, bg);
        if (status.length() > 11) status = status.substring(0, 11);
        d.drawString(status, r.x + r.w / 2, r.y + r.h - 10);
    }
}

static void drawTile(const Rect& r, const Tile& t, bool active) {
    auto& d = M5.Display;
    uint16_t bg = active ? COL_CARD_HI : COL_CARD;
    d.fillRoundRect(r.x, r.y, r.w, r.h, 6, bg);
    d.drawRoundRect(r.x, r.y, r.w, r.h, 6, active ? COL_ACCENT : COL_BORDER);

    // Compact tile (cells are ~56 px tall in the 4x3 layout): icon top, label
    // middle, status bottom.
    int iconX = r.x + (r.w - Icons::W) / 2;
    int iconY = r.y + 4;
    if (t.icon) Icons::draw(iconX, iconY, t.icon, COL_ACCENT);

    d.setFont(&fonts::Font2);
    d.setTextColor(COL_FG, bg);
    d.setTextDatum(top_center);
    d.drawString(t.label, r.x + r.w / 2, r.y + 22);

    drawTileStatus(r, t, active);
}

static void drawGrid(bool /*full*/) {
    s_tileHits.clear();
    auto& d = M5.Display;

    // Wipe the entire grid area so reflows after a tile-visibility change
    // don't leave ghosts behind.
    d.fillRect(0, kGridY, SCREEN_W, SCREEN_H - kGridY, COL_BG);

    // Build the visible-tile list according to the hide mask. We always show
    // Settings (the last entry) so the user can never lock themselves out of
    // editing the configuration on-device.
    uint32_t mask = Config::get().tileHideMask;
    int visible[kTileCount];
    int nVis = 0;
    for (int i = 0; i < kTileCount; i++) {
        bool isSettings = (kTiles[i].target == Screen::Settings);
        if (!isSettings && (mask & (1u << i))) continue;
        visible[nVis++] = i;
    }

    int cellW = (SCREEN_W - kGridGap * (kCols + 1)) / kCols;
    int cellH = (kGridH - kGridGap * (kRows + 1)) / kRows;
    for (int slot = 0; slot < nVis; slot++) {
        if (slot >= kCols * kRows) break;
        int row = slot / kCols;
        int col = slot % kCols;
        int x = kGridGap + col * (cellW + kGridGap);
        int y = kGridY + kGridGap + row * (cellH + kGridGap);
        Rect r { x, y, cellW, cellH };
        s_tileHits.push_back({ r, visible[slot] });
        drawTile(r, kTiles[visible[slot]], false);
    }
}

void draw(bool full) {
    if (full) {
        auto& d = M5.Display;
        d.fillScreen(COL_BG);
        drawBanner(true);
        drawGrid(true);
        s_lastHideMask = Config::get().tileHideMask;
        // Seed the per-tile status cache so subsequent partial redraws can
        // detect actual changes.
        for (int i = 0; i < kTileCount; i++) s_tileLastStatus[i] = "";
        for (auto& h : s_tileHits) {
            const Tile& t = kTiles[h.tileIdx];
            s_tileLastStatus[h.tileIdx] = t.statusFn ? t.statusFn() : String("");
        }
    }

    drawBanner(false);

    // Tile-visibility changes need a full grid relayout.
    uint32_t mask = Config::get().tileHideMask;
    if (mask != s_lastHideMask) {
        s_lastHideMask = mask;
        drawGrid(true);
        for (auto& h : s_tileHits) {
            const Tile& t = kTiles[h.tileIdx];
            s_tileLastStatus[h.tileIdx] = t.statusFn ? t.statusFn() : String("");
        }
        return;
    }

    // Otherwise refresh only the status strip of tiles whose text changed.
    // This avoids wiping the whole grid on every status tick (which is the
    // source of the visible flicker).
    for (auto& h : s_tileHits) {
        const Tile& t = kTiles[h.tileIdx];
        String cur = t.statusFn ? t.statusFn() : String("");
        if (cur != s_tileLastStatus[h.tileIdx]) {
            s_tileLastStatus[h.tileIdx] = cur;
            drawTileStatus(h.r, t, false);
        }
    }
}

void touch(int x, int y) {
    for (auto& h : s_tileHits) {
        if (h.r.contains(x, y)) {
            setScreen(kTiles[h.tileIdx].target);
            return;
        }
    }
}

}}
