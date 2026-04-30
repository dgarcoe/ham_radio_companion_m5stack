#include "ui_internal.h"

#include <vector>
#include "../alerts.h"
#include "../config.h"

namespace Ui { namespace ScreenAlerts {

enum class Mode { Rules, History };
static Mode s_mode = Mode::Rules;
static int s_lastDrawnRules = -1;
static int s_lastDrawnHistory = -1;
static bool s_lastMode = false;

static Rect s_btnRules { 0, 0, 0, 0 };
static Rect s_btnHist  { 0, 0, 0, 0 };

static void drawTopButtons() {
    int bw = 80, bh = 22;
    int by = CONTENT_Y + (HEADER_H - bh) / 2;
    int x1 = SCREEN_W - bw * 2 - 12;
    int x2 = SCREEN_W - bw - 8;
    s_btnRules = drawButton(x1, by, bw, bh, "Rules",
                            s_mode == Mode::Rules ? COL_ACCENT_D : COL_CARD,
                            s_mode == Mode::Rules ? COL_FG : COL_DIM);
    s_btnHist  = drawButton(x2, by, bw, bh, "History",
                            s_mode == Mode::History ? COL_ACCENT_D : COL_CARD,
                            s_mode == Mode::History ? COL_FG : COL_DIM);
}

static int rowsCardX()    { return 6; }
static int rowsCardW()    { return SCREEN_W - 12; }

struct ToggleHit { Rect r; int idx; };
static std::vector<ToggleHit> s_toggles;

static void drawRules() {
    auto& d = M5.Display;
    auto& cfg = Config::get();
    s_toggles.clear();

    int yTop = CONTENT_BODY_Y + 4;
    int avail = CONTENT_BODY_H - 8;
    d.fillRect(0, CONTENT_BODY_Y, SCREEN_W, CONTENT_BODY_H, COL_PANEL);

    if (cfg.alerts.empty()) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.drawString("No alert rules. Edit data/config.json and uploadfs.",
                     SCREEN_W / 2, yTop + avail / 2);
        return;
    }

    int rh = 32;
    int maxRows = avail / rh;
    for (int i = 0; i < (int)cfg.alerts.size() && i < maxRows; i++) {
        const AlertRule& r = cfg.alerts[i];
        int ry = yTop + i * rh;
        int cardX = rowsCardX(), cardW = rowsCardW();
        int cardH = rh - 4;
        drawCard(cardX, ry, cardW, cardH);

        // Toggle pill.
        int tw = 44, th = 20;
        int tx = cardX + 6;
        int ty = ry + (cardH - th) / 2;
        uint16_t bg = r.enabled ? COL_OK : COL_MUTED;
        Rect tr = drawButton(tx, ty, tw, th, r.enabled ? "ON" : "OFF",
                             bg, COL_BG);
        s_toggles.push_back({ tr, i });

        // Name.
        d.setFont(&fonts::Font2);
        d.setTextColor(COL_FG, COL_CARD);
        d.setTextDatum(top_left);
        d.drawString(r.name.length() ? r.name : String("(unnamed)"),
                     cardX + 60, ry + 4);

        // Sub-line with rule details.
        d.setFont(&fonts::Font0);
        d.setTextColor(COL_DIM, COL_CARD);
        String sub = String("band=") + (r.band.length() ? r.band : "any") +
                     "  mode=" + (r.mode.length() ? r.mode : "any") +
                     "  pfx=" + (r.prefix.length() ? r.prefix : "-") +
                     "  call~" + (r.callMatch.length() ? r.callMatch : "-");
        d.drawString(sub, cardX + 60, ry + 18);
    }
}

static void drawHistory() {
    auto& d = M5.Display;
    int yTop = CONTENT_BODY_Y + 4;
    int avail = CONTENT_BODY_H - 8;
    d.fillRect(0, CONTENT_BODY_Y, SCREEN_W, CONTENT_BODY_H, COL_PANEL);

    const auto& hits = Alerts::history();
    if (hits.empty()) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.drawString("No alerts yet.", SCREEN_W / 2, yTop + avail / 2);
        return;
    }

    int rh = 18;
    int rows = avail / rh;
    for (int i = 0; i < rows && i < (int)hits.size(); i++) {
        const AlertHit& h = hits[i];
        int ry = yTop + i * rh;
        if (i & 1) d.fillRect(0, ry, SCREEN_W, rh, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        uint16_t bg = (i & 1) ? COL_BG : COL_PANEL;
        d.setTextColor(COL_ACCENT, bg);
        d.drawString(h.spot.band, 8, ry + rh / 2);
        d.setTextColor(COL_FG, bg);
        char freq[16];
        snprintf(freq, sizeof(freq), "%.1f", h.spot.freqKHz);
        d.drawString(freq, 50, ry + rh / 2);
        d.drawString(h.spot.dx.substring(0, 10), 115, ry + rh / 2);
        d.setTextColor(COL_WARN, bg);
        d.drawString(h.spot.mode.substring(0, 4), 200, ry + rh / 2);
        d.setTextColor(COL_DIM, bg);
        d.drawString(h.ruleName.substring(0, 12), 245, ry + rh / 2);
    }
}

void draw(bool full) {
    if (full) {
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
    if (s_btnRules.contains(x, y)) { s_mode = Mode::Rules;   s_lastDrawnRules = -1;   return; }
    if (s_btnHist .contains(x, y)) { s_mode = Mode::History; s_lastDrawnHistory = -1; return; }

    if (s_mode == Mode::Rules) {
        for (auto& t : s_toggles) {
            if (t.r.contains(x, y)) {
                auto& cfg = Config::get();
                if (t.idx >= 0 && t.idx < (int)cfg.alerts.size()) {
                    cfg.alerts[t.idx].enabled = !cfg.alerts[t.idx].enabled;
                    Config::save();
                    s_lastDrawnRules = -1;
                }
                return;
            }
        }
    }
}

}}
