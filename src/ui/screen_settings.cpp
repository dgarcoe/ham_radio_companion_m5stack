#include "ui_internal.h"

#include "../config.h"
#include "../wifi_manager.h"
#include "../dx_cluster.h"

namespace Ui { namespace ScreenSettings {

struct Row {
    const char* label;
    String      (*get)();
    void        (*edit)();
};

static String getSsid()      { return Config::get().wifiSsid; }
static String getPass()      { String s; for (size_t i = 0; i < Config::get().wifiPass.length(); i++) s += '*'; return s; }
static String getCall()      { return Config::get().myCallsign; }
static String getHost()      { return Config::get().clusterHost; }
static String getPort()      { return String(Config::get().clusterPort); }
static String getOffset()    { return String((int)Config::get().utcOffset); }
static String getSound()     { return Config::get().soundEnabled ? "ON" : "OFF"; }
static String getPropUrl()   { return Config::get().propagationUrl; }

static void editSsid()    { auto& v = Config::get().wifiSsid;       if (Ui::editString("WiFi SSID", &v))      { Config::save(); WifiMgr::reconnect(); } }
static void editPass()    { auto& v = Config::get().wifiPass;       if (Ui::editString("WiFi Password", &v, true)) { Config::save(); WifiMgr::reconnect(); } }
static void editCall()    { auto& v = Config::get().myCallsign;     if (Ui::editString("My Callsign", &v))    { Config::save(); DxCluster::reconnect(); } }
static void editHost()    { auto& v = Config::get().clusterHost;    if (Ui::editString("DX Cluster Host", &v)){ Config::save(); DxCluster::reconnect(); } }
static void editPort()    {
    int p = Config::get().clusterPort;
    if (Ui::editInt("DX Cluster Port", &p, 1, 65535)) {
        Config::get().clusterPort = (uint16_t)p;
        Config::save();
        DxCluster::reconnect();
    }
}
static void editOffset()  { int v = Config::get().utcOffset; if (Ui::editInt("UTC offset (h)", &v, -12, 14)) { Config::get().utcOffset = (int8_t)v; Config::save(); } }
static void toggleSound() { Config::get().soundEnabled = !Config::get().soundEnabled; Config::save(); }
static void editPropUrl() { auto& v = Config::get().propagationUrl; if (Ui::editString("Propagation URL", &v, false, 80)) Config::save(); }

static const Row s_rows[] = {
    { "WiFi SSID",     getSsid,    editSsid    },
    { "WiFi Pass",     getPass,    editPass    },
    { "My Callsign",   getCall,    editCall    },
    { "Cluster Host",  getHost,    editHost    },
    { "Cluster Port",  getPort,    editPort    },
    { "UTC Offset",    getOffset,  editOffset  },
    { "Sound",         getSound,   toggleSound },
    { "Prop URL",      getPropUrl, editPropUrl },
};
static constexpr int kRowCount = sizeof(s_rows) / sizeof(s_rows[0]);
static int s_scroll = 0;
static String s_lastSig;

static int rowsVisible() { return (CONTENT_H - 4) / 24; }

static void drawRows() {
    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y + 24, SCREEN_W, CONTENT_H - 24, COL_BG);
    int rv = rowsVisible() - 1; // leave one for hint at bottom
    for (int i = 0; i < rv; i++) {
        int idx = s_scroll + i;
        if (idx >= kRowCount) break;
        const Row& r = s_rows[idx];
        int y = CONTENT_Y + 26 + i * 24;
        d.setTextColor(COL_DIM, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(top_left);
        d.drawString(r.label, 8, y);

        d.setTextColor(COL_FG, COL_BG);
        d.setTextDatum(top_right);
        String v = r.get();
        if (v.length() > 22) v = v.substring(0, 19) + "...";
        d.drawString(v, SCREEN_W - 36, y);

        // Edit chevron
        d.setTextColor(COL_ACCENT, COL_BG);
        d.setTextDatum(top_right);
        d.drawString(">", SCREEN_W - 8, y);
    }

    // Scroll hint
    int yBot = CONTENT_Y + CONTENT_H - 18;
    d.setFont(&fonts::Font0);
    d.setTextColor(COL_DIM, COL_BG);
    d.setTextDatum(top_center);
    d.drawString(
        s_scroll + rv < kRowCount ? "tap row to edit  -  swipe edge to scroll v"
                                  : "tap row to edit  -  swipe edge to scroll ^",
        SCREEN_W / 2, yBot);
}

void draw(bool full) {
    if (full) {
        clearContent();
        drawHeader("Settings");
        s_lastSig = "";
    }

    // Detect changes via a coarse signature.
    auto& cfg = Config::get();
    String sig = cfg.wifiSsid + "|" + String((int)cfg.wifiPass.length()) + "|" + cfg.myCallsign +
                 "|" + cfg.clusterHost + "|" + String(cfg.clusterPort) + "|" +
                 String((int)cfg.utcOffset) + "|" + (cfg.soundEnabled ? "1" : "0") +
                 "|" + cfg.propagationUrl + "|s=" + String(s_scroll);
    if (sig == s_lastSig) return;
    s_lastSig = sig;
    drawRows();
}

void touch(int x, int y) {
    int rv = rowsVisible() - 1;

    // Edge swipes for scrolling.
    if (x < 30) {
        s_scroll = max(0, s_scroll - 1);
        s_lastSig = "";
        return;
    }
    if (x > SCREEN_W - 30 && y > CONTENT_Y + CONTENT_H / 2) {
        if (s_scroll + rv < kRowCount) s_scroll++;
        s_lastSig = "";
        return;
    }

    int row = (y - (CONTENT_Y + 26)) / 24;
    if (row < 0 || row >= rv) return;
    int idx = s_scroll + row;
    if (idx < 0 || idx >= kRowCount) return;
    if (s_rows[idx].edit) {
        s_rows[idx].edit();
        s_lastSig = "";
        // After edit/keyboard the screen was wiped; force full repaint at next loop.
        Ui::setTab(Ui::Tab::Settings);   // no-op but ensures repaint flag handling
        // Easier: directly request full redraw via the public path.
        // The keyboard already cleared the screen, so trigger a full redraw:
        M5.Display.fillScreen(COL_BG);
        Ui::drawTabBar();
        drawHeader("Settings");
        drawRows();
    }
}

}}
