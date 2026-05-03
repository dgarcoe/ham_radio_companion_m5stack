#include "ui_internal.h"

#include <vector>
#include "../config.h"

namespace Ui { namespace ScreenHomeTiles {

// Mirrors the tile order in screen_launcher.cpp. Keeping this list in sync
// with kTiles there is part of the contract: bit N in tileHideMask hides the
// Nth entry below.
struct ToggleRow { const char* label; Screen screen; };

static const ToggleRow kRows[] = {
    { "DX Cluster",     Screen::DxCluster   },
    { "Propagation",    Screen::Propagation },
    { "NCDXF Beacons",  Screen::Beacons     },
    { "POTA",           Screen::Pota        },
    { "Alerts",         Screen::Alerts      },
    { "Bearing",        Screen::Bearing     },
    { "Space Weather",  Screen::Noaa        },
    { "Contest Cal.",   Screen::Contests    },
    { "Grayline",       Screen::Grayline    },
    { "Satellites",     Screen::Satellites  },
    { "SOTA",           Screen::Sota        },
    { "Settings",       Screen::Settings    },
};
static constexpr int kRowCount = sizeof(kRows) / sizeof(kRows[0]);
static const int kRowH = 26;

struct Hit { Rect r; int idx; };
static std::vector<Hit> s_hits;
static Rect s_btnUp   { 0, 0, 0, 0 };
static Rect s_btnDown { 0, 0, 0, 0 };
static int  s_scroll = 0;
static String s_lastSig;

static int rowsVisible() { return (CONTENT_BODY_H - 8) / kRowH; }

static void drawSubHeader() {
    auto& d = M5.Display;
    d.fillRect(0, CONTENT_Y, SCREEN_W, SUBHEADER_H, COL_PANEL);
    d.setFont(&fonts::Font2);
    d.setTextColor(COL_DIM, COL_PANEL);
    d.setTextDatum(middle_left);
    d.drawString("Tap to toggle. Settings always shown.", 8, CONTENT_Y + SUBHEADER_H / 2);

    int bw = 26, bh = 22;
    int by = CONTENT_Y + (SUBHEADER_H - bh) / 2;
    int x1 = SCREEN_W - bw * 2 - 14;
    int x2 = SCREEN_W - bw - 8;
    s_btnUp   = drawButton(x1, by, bw, bh, "^", COL_CARD, COL_FG);
    s_btnDown = drawButton(x2, by, bw, bh, "v", COL_CARD, COL_FG);
}

static void drawRows() {
    auto& d = M5.Display;
    s_hits.clear();
    d.fillRect(0, CONTENT_BODY_Y, SCREEN_W, CONTENT_BODY_H, COL_PANEL);

    uint32_t mask = Config::get().tileHideMask;
    int yTop = CONTENT_BODY_Y + 4;
    int rv = rowsVisible();
    for (int i = 0; i < rv; i++) {
        int idx = s_scroll + i;
        if (idx >= kRowCount) break;
        const ToggleRow& r = kRows[idx];
        bool isSettings = (r.screen == Screen::Settings);
        bool hidden = (mask & (1u << idx)) != 0;

        int y = yTop + i * kRowH;
        int cardX = 6;
        int cardW = SCREEN_W - 12;
        int cardH = kRowH - 4;
        drawCard(cardX, y, cardW, cardH);

        d.setFont(&fonts::Font2);
        d.setTextColor(COL_FG, COL_CARD);
        d.setTextDatum(middle_left);
        d.drawString(r.label, cardX + 10, y + cardH / 2);

        // Right-side ON/OFF pill (locked for Settings).
        const char* label;
        uint16_t bg, fg;
        if (isSettings) { label = "LOCK"; bg = COL_MUTED;  fg = COL_DIM; }
        else if (hidden){ label = "OFF";  bg = COL_CARD_HI; fg = COL_DIM; }
        else            { label = "ON";   bg = COL_ACCENT_D; fg = COL_FG; }

        int pw = 56, ph = cardH - 4;
        int px = cardX + cardW - pw - 4;
        int py = y + 2;
        drawButton(px, py, pw, ph, label, bg, fg);

        s_hits.push_back({ { cardX, y, cardW, cardH }, idx });
    }
}

void draw(bool full) {
    if (full) s_lastSig = "";

    String sig = String(Config::get().tileHideMask, HEX) + "|s=" + String(s_scroll);
    if (sig == s_lastSig) return;
    s_lastSig = sig;

    drawSubHeader();
    drawRows();
}

void touch(int x, int y) {
    int rv = rowsVisible();
    if (s_btnUp.contains(x, y))   { if (s_scroll > 0) s_scroll--; s_lastSig = ""; return; }
    if (s_btnDown.contains(x, y)) { if (s_scroll + rv < kRowCount) s_scroll++; s_lastSig = ""; return; }

    for (auto& h : s_hits) {
        if (h.r.contains(x, y)) {
            const ToggleRow& r = kRows[h.idx];
            if (r.screen == Screen::Settings) return; // locked
            uint32_t& mask = Config::get().tileHideMask;
            mask ^= (1u << h.idx);
            Config::save();
            s_lastSig = "";
            return;
        }
    }
}

}}
