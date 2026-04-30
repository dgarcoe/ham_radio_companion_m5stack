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

namespace Ui { namespace ScreenLauncher {

// Top banner (clock + callsign + date) followed by a 3x2 tile grid.

static const int kBannerH = 56;

static const int kGridY = kBannerH + 4;
static const int kGridH = SCREEN_H - kGridY - 4;
static const int kCols = 3;
static const int kRows = 2;
static const int kGridGap = 6;

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
static String settingsStatus(){ return Config::loadSource(); }

static const Tile kTiles[] = {
    { Screen::DxCluster,   "DX Cluster", Icons::DxIcon,       dxStatus       },
    { Screen::Propagation, "Propagation",Icons::PropIcon,     propStatus     },
    { Screen::Alerts,      "Alerts",     Icons::AlertsIcon,   alertsStatus   },
    { Screen::Beacons,     "Beacons",    Icons::BeaconIcon,   beaconsStatus  },
    { Screen::Pota,        "POTA",       Icons::PotaIcon,     potaStatus     },
    { Screen::Settings,    "Settings",   Icons::SettingsIcon, settingsStatus },
};
static constexpr int kTileCount = sizeof(kTiles) / sizeof(kTiles[0]);

static std::vector<Rect> s_tileRects;
static String s_lastSig;
static String s_lastClock;
static String s_lastCall;

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

    // Callsign + date on the right.
    if (cfg.myCallsign != s_lastCall || full) {
        s_lastCall = cfg.myCallsign;
        d.fillRect(186, 4, SCREEN_W - 186 - 6, kBannerH - 8, COL_BG);
        d.setFont(&fonts::Font4);
        d.setTextColor(COL_FG, COL_BG);
        d.setTextDatum(top_right);
        d.drawString(cfg.myCallsign, SCREEN_W - 8, 6);

        d.setFont(&fonts::Font0);
        d.setTextColor(COL_DIM, COL_BG);
        d.setTextDatum(top_right);
        d.drawString(formatDate(), SCREEN_W - 8, 38);
    }
}

static void drawTile(const Rect& r, const Tile& t, bool active) {
    auto& d = M5.Display;
    uint16_t bg = active ? COL_CARD_HI : COL_CARD;
    d.fillRoundRect(r.x, r.y, r.w, r.h, 8, bg);
    d.drawRoundRect(r.x, r.y, r.w, r.h, 8, active ? COL_ACCENT : COL_BORDER);

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
        d.drawString(status, r.x + r.w / 2, r.y + r.h - 14);
    }
}

static void drawGrid(bool full) {
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
    (void)full;
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

    // Status fields can change asynchronously; rebuild a coarse signature so we
    // only redraw the grid when something actually changed.
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
