#include "ui_internal.h"

#include "../alerts.h"
#include "../config.h"

namespace Ui { namespace ScreenAlerts {

enum class Mode { Rules, History };
static Mode s_mode = Mode::Rules;
static int s_lastDrawnRules = -1;
static int s_lastDrawnHistory = -1;
static bool s_lastMode = false;

static void drawTopButtons() {
    auto& d = M5.Display;
    int bw = 80, bh = 22;
    int y = CONTENT_Y + 2;
    d.fillRoundRect(80,  y, bw, bh, 4, s_mode == Mode::Rules   ? COL_TAB_SEL : COL_TAB_BG);
    d.fillRoundRect(170, y, bw, bh, 4, s_mode == Mode::History ? COL_TAB_SEL : COL_TAB_BG);
    d.setTextColor(COL_FG, COL_BG);
    d.setTextDatum(middle_center);
    d.setFont(&fonts::Font2);
    d.drawString("Rules",   80  + bw / 2, y + bh / 2);
    d.drawString("History", 170 + bw / 2, y + bh / 2);
}

static void drawRules() {
    auto& d = M5.Display;
    auto& cfg = Config::get();
    int y = CONTENT_Y + 30;
    d.fillRect(0, y, SCREEN_W, CONTENT_H - 30, COL_BG);

    if (cfg.alerts.empty()) {
        d.setTextColor(COL_DIM, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.drawString("No alert rules. Edit via Settings or serial.", SCREEN_W / 2, y + 40);
        return;
    }

    int rh = 30;
    for (size_t i = 0; i < cfg.alerts.size() && (int)i < 6; i++) {
        const AlertRule& r = cfg.alerts[i];
        int ry = y + (int)i * rh;
        // Toggle pad
        uint16_t col = r.enabled ? COL_OK : COL_DIM;
        d.fillRoundRect(6, ry + 4, 40, 22, 4, col);
        d.setTextColor(COL_BG, col);
        d.setTextDatum(middle_center);
        d.setFont(&fonts::Font2);
        d.drawString(r.enabled ? "ON" : "OFF", 26, ry + 15);

        d.setTextColor(COL_FG, COL_BG);
        d.setTextDatum(top_left);
        d.drawString(r.name.length() ? r.name : String("(unnamed)"), 56, ry + 2);

        d.setTextColor(COL_DIM, COL_BG);
        String sub = String("band=") + (r.band.length() ? r.band : "any") +
                     "  mode=" + (r.mode.length() ? r.mode : "any") +
                     "  pfx=" + (r.prefix.length() ? r.prefix : "-") +
                     "  call~" + (r.callMatch.length() ? r.callMatch : "-");
        d.drawString(sub, 56, ry + 16);
    }
}

static void drawHistory() {
    auto& d = M5.Display;
    int y = CONTENT_Y + 30;
    d.fillRect(0, y, SCREEN_W, CONTENT_H - 30, COL_BG);

    const auto& hits = Alerts::history();
    if (hits.empty()) {
        d.setTextColor(COL_DIM, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.drawString("No alerts yet.", SCREEN_W / 2, y + 40);
        return;
    }
    int rh = 18;
    int rows = (CONTENT_H - 34) / rh;
    for (int i = 0; i < rows && i < (int)hits.size(); i++) {
        const AlertHit& h = hits[i];
        int ry = y + i * rh;
        d.setFont(&fonts::Font2);
        d.setTextDatum(top_left);
        d.setTextColor(COL_ACCENT, COL_BG);
        d.drawString(h.spot.band, 6, ry);
        d.setTextColor(COL_FG, COL_BG);
        char freq[16];
        snprintf(freq, sizeof(freq), "%.1f", h.spot.freqKHz);
        d.drawString(freq, 50, ry);
        d.drawString(h.spot.dx.substring(0, 10), 110, ry);
        d.setTextColor(COL_WARN, COL_BG);
        d.drawString(h.spot.mode.substring(0, 4), 195, ry);
        d.setTextColor(COL_DIM, COL_BG);
        d.drawString(h.ruleName.substring(0, 10), 235, ry);
    }
}

void draw(bool full) {
    if (full) {
        clearContent();
        drawHeader("Alerts");
        drawTopButtons();
        s_lastDrawnRules = -1;
        s_lastDrawnHistory = -1;
        s_lastMode = (s_mode == Mode::History);
    }

    if (s_mode == Mode::Rules) {
        int sig = (int)Config::get().alerts.size();
        for (auto& r : Config::get().alerts) sig = sig * 31 + (r.enabled ? 1 : 0);
        if (sig != s_lastDrawnRules || s_lastMode != false) {
            s_lastDrawnRules = sig;
            s_lastMode = false;
            drawTopButtons();
            drawRules();
        }
    } else {
        int sig = (int)Alerts::history().size();
        if (sig != s_lastDrawnHistory || s_lastMode != true) {
            s_lastDrawnHistory = sig;
            s_lastMode = true;
            drawTopButtons();
            drawHistory();
        }
    }
}

void touch(int x, int y) {
    // Mode buttons
    if (y < CONTENT_Y + 26) {
        if (x >= 80  && x <= 160) { s_mode = Mode::Rules;   s_lastDrawnRules = -1; }
        if (x >= 170 && x <= 250) { s_mode = Mode::History; s_lastDrawnHistory = -1; }
        return;
    }

    if (s_mode == Mode::Rules) {
        auto& cfg = Config::get();
        int rh = 30;
        int yTop = CONTENT_Y + 30;
        for (size_t i = 0; i < cfg.alerts.size() && (int)i < 6; i++) {
            int ry = yTop + (int)i * rh;
            if (y >= ry + 4 && y <= ry + 26 && x >= 6 && x <= 46) {
                cfg.alerts[i].enabled = !cfg.alerts[i].enabled;
                Config::save();
                s_lastDrawnRules = -1;
                break;
            }
        }
    }
}

}}
