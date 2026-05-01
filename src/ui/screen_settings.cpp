#include "ui_internal.h"

#include <vector>
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
static String getGrid()      { return Config::get().myGrid; }
static String getHost()      { return Config::get().clusterHost; }
static String getPort()      { return String(Config::get().clusterPort); }
static String getOffset()    { return String((int)Config::get().utcOffset); }
static String getSound()     { return Config::get().soundEnabled ? "ON" : "OFF"; }
static String getPropUrl()   { return Config::get().propagationUrl; }

static void editSsid()    { auto& v = Config::get().wifiSsid;       if (Ui::editString("WiFi SSID", &v))      { Config::save(); WifiMgr::reconnect(); } }
static void editPass()    { auto& v = Config::get().wifiPass;       if (Ui::editString("WiFi Password", &v, true)) { Config::save(); WifiMgr::reconnect(); } }
static void editCall()    { auto& v = Config::get().myCallsign;     if (Ui::editString("My Callsign", &v))    { Config::save(); DxCluster::reconnect(); } }
static void editGrid()    { auto& v = Config::get().myGrid;         if (Ui::editString("My QTH grid (FN30as)", &v, false, 8)) { v.trim(); Config::save(); } }
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
    { "My Grid",       getGrid,    editGrid    },
    { "Cluster Host",  getHost,    editHost    },
    { "Cluster Port",  getPort,    editPort    },
    { "UTC Offset",    getOffset,  editOffset  },
    { "Sound",         getSound,   toggleSound },
    { "Prop URL",      getPropUrl, editPropUrl },
};
static constexpr int kRowCount = sizeof(s_rows) / sizeof(s_rows[0]);
static int s_scroll = 0;
static String s_lastSig;
static const int kRowH = 26;

struct RowHit { Rect r; int idx; };
static std::vector<RowHit> s_rowHits;
static Rect s_btnUp { 0, 0, 0, 0 };
static Rect s_btnDown { 0, 0, 0, 0 };

static int rowsVisible() {
    return (CONTENT_BODY_H - 8) / kRowH;
}

static void drawScrollButtons() {
    int bw = 26, bh = 22;
    int by = CONTENT_Y + (SUBHEADER_H - bh) / 2;
    int x1 = SCREEN_W - bw * 2 - 14;
    int x2 = SCREEN_W - bw - 8;
    s_btnUp   = drawButton(x1, by, bw, bh, "^", COL_CARD, COL_FG);
    s_btnDown = drawButton(x2, by, bw, bh, "v", COL_CARD, COL_FG);
}

static void drawRows() {
    auto& d = M5.Display;
    s_rowHits.clear();
    d.fillRect(0, CONTENT_BODY_Y, SCREEN_W, CONTENT_BODY_H, COL_PANEL);

    int yTop = CONTENT_BODY_Y + 4;
    int rv = rowsVisible();
    for (int i = 0; i < rv; i++) {
        int idx = s_scroll + i;
        if (idx >= kRowCount) break;
        const Row& r = s_rows[idx];
        int y = yTop + i * kRowH;
        int cardX = 6;
        int cardW = SCREEN_W - 12;
        int cardH = kRowH - 4;
        drawCard(cardX, y, cardW, cardH);

        d.setFont(&fonts::Font2);
        d.setTextColor(COL_DIM, COL_CARD);
        d.setTextDatum(middle_left);
        d.drawString(r.label, cardX + 10, y + cardH / 2);

        d.setTextColor(COL_FG, COL_CARD);
        d.setTextDatum(middle_right);
        String v = r.get();
        if (v.length() > 22) v = v.substring(0, 19) + "...";
        d.drawString(v, cardX + cardW - 18, y + cardH / 2);

        d.setTextColor(COL_ACCENT, COL_CARD);
        d.setTextDatum(middle_right);
        d.drawString(">", cardX + cardW - 4, y + cardH / 2);

        s_rowHits.push_back({ { cardX, y, cardW, cardH }, idx });
    }
}

void draw(bool full) {
    if (full) {
        drawScrollButtons();
        s_lastSig = "";
    }

    auto& cfg = Config::get();
    String sig = cfg.wifiSsid + "|" + String((int)cfg.wifiPass.length()) + "|" + cfg.myCallsign +
                 "|" + cfg.clusterHost + "|" + String(cfg.clusterPort) + "|" +
                 String((int)cfg.utcOffset) + "|" + (cfg.soundEnabled ? "1" : "0") +
                 "|" + cfg.propagationUrl + "|s=" + String(s_scroll);
    if (sig == s_lastSig) return;
    s_lastSig = sig;
    drawRows();
}

static void forceFullRedrawAfterModal() {
    // The modal keyboard cleared the screen. Letting the UI loop run its full
    // redraw path is the simplest way to put the chrome back consistently.
    Ui::requestFullRedraw();
    s_lastSig = "";
}

void touch(int x, int y) {
    int rv = rowsVisible();
    if (s_btnUp.contains(x, y)) {
        s_scroll = std::max(0, s_scroll - 1);
        s_lastSig = "";
        return;
    }
    if (s_btnDown.contains(x, y)) {
        if (s_scroll + rv < kRowCount) s_scroll++;
        s_lastSig = "";
        return;
    }

    for (auto& h : s_rowHits) {
        if (h.r.contains(x, y)) {
            if (h.idx >= 0 && h.idx < kRowCount && s_rows[h.idx].edit) {
                s_rows[h.idx].edit();
                forceFullRedrawAfterModal();
            }
            return;
        }
    }
}

}}
