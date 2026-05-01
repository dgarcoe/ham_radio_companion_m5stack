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

namespace Ui { namespace ScreenLauncher {

// Top banner (clock + callsign + date + battery) followed by a 4x2 tile grid.

static const int kBannerH = 56;

static const int kGridY = kBannerH + 4;
static const int kGridH = SCREEN_H - kGridY - 4;
static const int kCols = 4;
static const int kRows = 2;
static const int kGridGap = 5;

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
    return String(active) + " on / " + String((unsigned)Alerts::history().size()) + " hits";
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
static String settingsStatus(){ return Config::loadSource(); }

static const Tile kTiles[] = {
    { Screen::DxCluster,   "DX",         Icons::DxIcon,       dxStatus       },
    { Screen::Propagation, "Prop",       Icons::PropIcon,     propStatus     },
    { Screen::Beacons,     "Beacons",    Icons::BeaconIcon,   beaconsStatus  },
    { Screen::Pota,        "POTA",       Icons::PotaIcon,     potaStatus     },
    { Screen::Alerts,      "Alerts",     Icons::AlertsIcon,   alertsStatus   },
    { Screen::Bearing,     "Bearing",    Icons::BearingIcon,  bearingStatus  },
    { Screen::Noaa,        "Sp.Wx",      Icons::NoaaIcon,     noaaStatus     },
    { Screen::Settings,    "Settings",   Icons::SettingsIcon, settingsStatus },
};
static constexpr int kTileCount = sizeof(kTiles) / sizeof(kTiles[0]);

static std::vector<Rect> s_tileRects;
static String s_lastSig;
static String s_lastClock;
static String s_lastCall;
static int    s_lastBatteryLevel = -1;
static int    s_lastBatteryCharging = -1;

static String formatClock() {
    time_t now = time(nullptr);
    if (now < 1700000000) return "--:--:--";
    struct tm tmv;
    gmtime_r(&now, &tmv);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
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

    // Big clock on the left.
    String clk = formatClock();
    if (clk != s_lastClock) {
        s_lastClock = clk;
        d.fillRect(6, 4, 180, kBannerH - 8, COL_BG);
        d.setFont(&fonts::Font7);
        d.setTextColor(COL_ACCENT, COL_BG);
        d.setTextDatum(top_left);
        d.drawString(clk, 6, 4);
    }

    // Callsign + date in the middle-right area, leaving space for the battery.
    int textRight = SCREEN_W - kBatRightMargin;
    int textLeft  = 188;
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

static void drawTile(const Rect& r, const Tile& t, bool active) {
    auto& d = M5.Display;
    uint16_t bg = active ? COL_CARD_HI : COL_CARD;
    d.fillRoundRect(r.x, r.y, r.w, r.h, 6, bg);
    d.drawRoundRect(r.x, r.y, r.w, r.h, 6, active ? COL_ACCENT : COL_BORDER);

    int iconX = r.x + (r.w - Icons::W) / 2;
    int iconY = r.y + 8;
    if (t.icon) Icons::draw(iconX, iconY, t.icon, COL_ACCENT);

    d.setFont(&fonts::Font2);
    d.setTextColor(COL_FG, bg);
    d.setTextDatum(top_center);
    d.drawString(t.label, r.x + r.w / 2, r.y + 28);

    String status = t.statusFn ? t.statusFn() : String("");
    if (status.length()) {
        d.setFont(&fonts::Font0);
        d.setTextColor(COL_DIM, bg);
        // Truncate to fit narrower 4-wide tile (~75 px / 6 = 12 chars).
        if (status.length() > 13) status = status.substring(0, 13);
        d.drawString(status, r.x + r.w / 2, r.y + r.h - 12);
    }
}

static void drawGrid(bool /*full*/) {
    s_tileRects.clear();
    int cellW = (SCREEN_W - kGridGap * (kCols + 1)) / kCols;
    int cellH = (kGridH - kGridGap * (kRows + 1)) / kRows;
    for (int row = 0; row < kRows; row++) {
        for (int col = 0; col < kCols; col++) {
            int idx = row * kCols + col;
            if (idx >= kTileCount) break;
            int x = kGridGap + col * (cellW + kGridGap);
            int y = kGridY + kGridGap + row * (cellH + kGridGap);
            Rect r { x, y, cellW, cellH };
            s_tileRects.push_back(r);
            drawTile(r, kTiles[idx], false);
        }
    }
}

void draw(bool full) {
    if (full) {
        auto& d = M5.Display;
        d.fillScreen(COL_BG);
        drawBanner(true);
        drawGrid(true);
        s_lastSig = "";
    }

    drawBanner(false);

    String sig;
    for (auto& t : kTiles) sig += (t.statusFn ? t.statusFn() : String("")) + "|";
    if (sig != s_lastSig) {
        s_lastSig = sig;
        drawGrid(false);
    }
}

void touch(int x, int y) {
    for (size_t i = 0; i < s_tileRects.size() && i < kTileCount; i++) {
        if (s_tileRects[i].contains(x, y)) {
            setScreen(kTiles[i].target);
            return;
        }
    }
}

}}
