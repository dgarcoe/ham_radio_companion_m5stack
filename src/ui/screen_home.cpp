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
static String s_lastCall;
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

// Layout: card 1 (clock + callsign + date) on top, card 2 (status) below.
static const int kCardX = 6;
static const int kCardW = SCREEN_W - 12;

static const int kClockCardY = CONTENT_BODY_Y + 4;
static const int kClockCardH = 100;

static const int kClockY    = kClockCardY + 4;     // big clock top
static const int kCallsignY = kClockCardY + 58;    // callsign row top
static const int kDateY     = kClockCardY + 86;    // date row top

static const int kStatusCardY = kClockCardY + kClockCardH + 4;
static const int kStatusCardH = CONTENT_BODY_H - (kClockCardH + 12);

// Pick a font for the callsign that fits the card width.
static const lgfx::IFont* fontForCallsign(const String& s) {
    auto& d = M5.Display;
    int avail = kCardW - 24;
    d.setFont(&fonts::Font4);
    if (d.textWidth(s) <= avail) return &fonts::Font4;
    d.setFont(&fonts::Font2);
    return &fonts::Font2;
}

static void drawStatusRow(int slot, const char* label, const String& value, uint16_t valColor) {
    auto& d = M5.Display;
    int rowH = (kStatusCardH - 8) / 3;
    int y = kStatusCardY + 4 + slot * rowH;
    d.fillRect(kCardX + 2, y, kCardW - 4, rowH - 1, COL_CARD);
    d.setFont(&fonts::Font2);
    d.setTextDatum(middle_left);
    d.setTextColor(COL_DIM, COL_CARD);
    d.drawString(label, kCardX + 12, y + rowH / 2);
    d.setTextColor(valColor, COL_CARD);
    d.drawString(value, kCardX + 70, y + rowH / 2);
}

void draw(bool full) {
    auto& d = M5.Display;
    auto& cfg = Config::get();

    if (full) {
        clearContent();
        drawHeader("Home");

        drawCard(kCardX, kClockCardY,  kCardW, kClockCardH);
        drawCard(kCardX, kStatusCardY, kCardW, kStatusCardH);

        s_lastClock = "";
        s_lastDate  = "";
        s_lastCall  = "";
        s_lastWifi  = "";
        s_lastDx    = "";
        s_lastProp  = "";
    }

    // Big UTC clock - centered horizontally.
    String clk = formatClock();
    if (clk != s_lastClock) {
        s_lastClock = clk;
        d.fillRect(kCardX + 2, kClockY, kCardW - 4, 50, COL_CARD);
        d.setTextColor(COL_ACCENT, COL_CARD);
        d.setFont(&fonts::Font7);
        d.setTextDatum(top_center);
        d.drawString(clk, kCardX + kCardW / 2, kClockY);
    }

    // Callsign row - centered, with auto-shrink for long calls. Drawn below
    // the clock so it never collides with it.
    if (cfg.myCallsign != s_lastCall) {
        s_lastCall = cfg.myCallsign;
        d.fillRect(kCardX + 2, kCallsignY, kCardW - 4, 26, COL_CARD);
        const auto* font = fontForCallsign(cfg.myCallsign);
        d.setFont(font);
        d.setTextColor(COL_FG, COL_CARD);
        d.setTextDatum(top_center);
        d.drawString(cfg.myCallsign, kCardX + kCardW / 2, kCallsignY);
    }

    // Date row.
    String dat = formatDate();
    if (dat != s_lastDate) {
        s_lastDate = dat;
        d.fillRect(kCardX + 2, kDateY, kCardW - 4, 14, COL_CARD);
        d.setFont(&fonts::Font0);
        d.setTextColor(COL_MUTED, COL_CARD);
        d.setTextDatum(top_center);
        d.drawString(dat, kCardX + kCardW / 2, kDateY);
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
