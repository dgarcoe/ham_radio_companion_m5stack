#include "ui_internal.h"

#include <WiFi.h>
#include <time.h>
#include "../config.h"
#include "../wifi_manager.h"
#include "../dx_cluster.h"
#include "../propagation.h"

namespace Ui { namespace ScreenHome {

static String s_lastClock;
static String s_lastDate;
static String s_lastWifi;
static String s_lastDx;
static String s_lastProp;

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
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d UTC", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
    return String(buf);
}

static void drawValueLine(int y, const char* label, const String& value, uint16_t valColor) {
    auto& d = M5.Display;
    d.fillRect(0, y, SCREEN_W, 16, COL_BG);
    d.setFont(&fonts::Font2);
    d.setTextDatum(top_left);
    d.setTextColor(COL_DIM, COL_BG);
    d.drawString(label, 8, y);
    d.setTextColor(valColor, COL_BG);
    d.drawString(value, 88, y);
}

void draw(bool full) {
    auto& d = M5.Display;
    auto& cfg = Config::get();

    if (full) {
        clearContent();
        drawHeader("Home");

        // Big callsign
        d.setTextColor(COL_FG, COL_BG);
        d.setFont(&fonts::Font4);
        d.setTextDatum(top_left);
        d.drawString(cfg.myCallsign, 8, CONTENT_Y + 28);

        s_lastClock = ""; s_lastDate = "";
        s_lastWifi = ""; s_lastDx = ""; s_lastProp = "";
    }

    // Big clock (UTC)
    String clk = formatClock();
    if (clk != s_lastClock) {
        s_lastClock = clk;
        d.fillRect(140, CONTENT_Y + 26, 180, 34, COL_BG);
        d.setTextColor(COL_ACCENT, COL_BG);
        d.setFont(&fonts::Font7);
        d.setTextDatum(top_right);
        d.drawString(clk, SCREEN_W - 6, CONTENT_Y + 26);
    }

    String dat = formatDate();
    if (dat != s_lastDate) {
        s_lastDate = dat;
        d.fillRect(140, CONTENT_Y + 64, 180, 16, COL_BG);
        d.setTextColor(COL_DIM, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(top_right);
        d.drawString(dat, SCREEN_W - 6, CONTENT_Y + 64);
    }

    // Status block
    int y = CONTENT_Y + 96;

    String wifiLine;
    uint16_t wifiColor;
    if (WifiMgr::isConnected()) {
        wifiLine = WifiMgr::ssid() + "  " + WifiMgr::ip() + "  " + String(WifiMgr::rssi()) + "dBm";
        wifiColor = COL_OK;
    } else {
        wifiLine = cfg.wifiSsid.length() ? "connecting..." : "(not configured)";
        wifiColor = COL_WARN;
    }
    if (wifiLine != s_lastWifi) {
        s_lastWifi = wifiLine;
        drawValueLine(y, "WiFi:", wifiLine, wifiColor);
    }

    String dxLine = DxCluster::isConnected()
        ? (String("connected, ") + String((unsigned)DxCluster::spotCount()) + " spots")
        : DxCluster::status();
    uint16_t dxColor = DxCluster::isConnected() ? COL_OK : COL_WARN;
    if (dxLine != s_lastDx) {
        s_lastDx = dxLine;
        drawValueLine(y + 18, "DXC:", dxLine, dxColor);
    }

    const auto& p = Propagation::data();
    String propLine;
    uint16_t propColor;
    if (p.valid) {
        propLine = String("SFI ") + p.solarFlux + "  A " + p.aIndex + "  K " + p.kIndex;
        propColor = COL_OK;
    } else {
        propLine = Propagation::status();
        propColor = COL_DIM;
    }
    if (propLine != s_lastProp) {
        s_lastProp = propLine;
        drawValueLine(y + 36, "Sun:", propLine, propColor);
    }
}

void touch(int, int) { /* no interactive elements */ }

}}
