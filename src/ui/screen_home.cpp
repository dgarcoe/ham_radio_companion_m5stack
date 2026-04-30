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

// Geometry: clock card on top spanning full width, then a status card below.
static const int kCardX = 8;
static const int kCardW = SCREEN_W - 16;
static const int kClockY = CONTENT_BODY_Y + 4;
static const int kClockH = 78;
static const int kStatusY = kClockY + kClockH + 6;
static const int kStatusH = CONTENT_BODY_H - (kClockY - CONTENT_BODY_Y) - kClockH - 12;

static void drawStatusRow(int slot, const char* label, const String& value, uint16_t valColor) {
    auto& d = M5.Display;
    int rowH = (kStatusH - 8) / 3;
    int y = kStatusY + 4 + slot * rowH;
    d.fillRect(kCardX + 2, y, kCardW - 4, rowH - 2, COL_CARD);
    d.setFont(&fonts::Font2);
    d.setTextDatum(middle_left);
    d.setTextColor(COL_DIM, COL_CARD);
    d.drawString(label, kCardX + 10, y + rowH / 2);
    d.setTextColor(valColor, COL_CARD);
    d.drawString(value, kCardX + 78, y + rowH / 2);
}

void draw(bool full) {
    auto& d = M5.Display;
    auto& cfg = Config::get();

    if (full) {
        clearContent();
        drawHeader("Home");

        // Clock card: callsign on the left, clock+date on the right.
        drawCard(kCardX, kClockY, kCardW, kClockH);
        d.setTextColor(COL_FG, COL_CARD);
        d.setFont(&fonts::Font4);
        d.setTextDatum(middle_left);
        d.drawString(cfg.myCallsign, kCardX + 12, kClockY + 30);

        d.setTextColor(COL_MUTED, COL_CARD);
        d.setFont(&fonts::Font0);
        d.drawString("YOUR CALL", kCardX + 12, kClockY + 12);

        // Status card frame.
        drawCard(kCardX, kStatusY, kCardW, kStatusH);

        s_lastClock = ""; s_lastDate = "";
        s_lastWifi = ""; s_lastDx = ""; s_lastProp = "";
    }

    // Big clock (UTC) on the right side of the clock card.
    String clk = formatClock();
    if (clk != s_lastClock) {
        s_lastClock = clk;
        d.fillRect(kCardX + 130, kClockY + 8, kCardW - 130 - 8, 44, COL_CARD);
        d.setTextColor(COL_ACCENT, COL_CARD);
        d.setFont(&fonts::Font7);
        d.setTextDatum(top_right);
        d.drawString(clk, kCardX + kCardW - 8, kClockY + 8);
    }

    String dat = formatDate();
    if (dat != s_lastDate) {
        s_lastDate = dat;
        d.fillRect(kCardX + 130, kClockY + 56, kCardW - 130 - 8, 16, COL_CARD);
        d.setTextColor(COL_DIM, COL_CARD);
        d.setFont(&fonts::Font2);
        d.setTextDatum(top_right);
        d.drawString(dat, kCardX + kCardW - 8, kClockY + 56);
    }

    // Status block.
    String wifiLine;
    uint16_t wifiColor;
    if (WifiMgr::isConnected()) {
        wifiLine = WifiMgr::ssid() + "  " + WifiMgr::ip();
        wifiColor = COL_OK;
    } else {
        wifiLine = cfg.wifiSsid.length() ? "connecting..." : "(not configured)";
        wifiColor = COL_WARN;
    }
    if (wifiLine != s_lastWifi) {
        s_lastWifi = wifiLine;
        drawStatusRow(0, "WiFi", wifiLine, wifiColor);
    }

    String dxLine = DxCluster::isConnected()
        ? (String("connected, ") + String((unsigned)DxCluster::spotCount()) + " spots")
        : DxCluster::status();
    uint16_t dxColor = DxCluster::isConnected() ? COL_OK : COL_WARN;
    if (dxLine != s_lastDx) {
        s_lastDx = dxLine;
        drawStatusRow(1, "DXC", dxLine, dxColor);
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
        drawStatusRow(2, "Sun", propLine, propColor);
    }
}

void touch(int, int) { /* no interactive elements */ }

}}
