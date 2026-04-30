#include "ui_internal.h"
#include "icons.h"

#include "../propagation.h"

namespace Ui { namespace ScreenProp {

static String s_lastSig;
static Rect s_refreshBtn { 0, 0, 0, 0 };

static uint16_t condColor(const String& c) {
    String s = c; s.toLowerCase();
    if (s.indexOf("good") >= 0) return COL_OK;
    if (s.indexOf("fair") >= 0) return COL_WARN;
    if (s.indexOf("poor") >= 0) return COL_BAD;
    return COL_DIM;
}

static void drawRefreshButton() {
    int bw = 96, bh = 22;
    int bx = SCREEN_W - bw - 8;
    int by = CONTENT_Y + (HEADER_H - bh) / 2;
    s_refreshBtn = drawIconButton(bx, by, bw, bh, "Refresh",
                                  Icons::RefreshIcon, COL_ACCENT_D, COL_FG);
}

static void drawMetricCell(int x, int y, int w, int h, const char* label, const String& value) {
    auto& d = M5.Display;
    drawCard(x, y, w, h, COL_CARD);
    d.setFont(&fonts::Font0);
    d.setTextColor(COL_MUTED, COL_CARD);
    d.setTextDatum(top_left);
    d.drawString(label, x + 6, y + 4);

    d.setFont(&fonts::Font4);
    d.setTextColor(COL_FG, COL_CARD);
    d.setTextDatum(middle_center);
    String shown = value.length() ? value : String("-");
    if (d.textWidth(shown) > w - 8) {
        // Fall back to a smaller font if the value would overflow the cell.
        d.setFont(&fonts::Font2);
    }
    d.drawString(shown, x + w / 2, y + h / 2 + 6);
}

void draw(bool full) {
    auto& d = M5.Display;
    const auto& p = Propagation::data();

    if (full) {
        // Top header is rendered by ui.cpp; we just paint our action button.
        drawRefreshButton();
        s_lastSig = "";
    }

    String sig = p.valid
        ? (p.solarFlux + "|" + p.aIndex + "|" + p.kIndex + "|" + p.sunspots +
           "|" + p.xrayClass + "|" + p.solarWind + "|" + p.muf + "|" + p.hf +
           "|" + p.signalNoise + "|" + p.aurora + "|" + String((int)p.bands.size()))
        : ("invalid|" + Propagation::status());
    if (sig == s_lastSig) return;
    s_lastSig = sig;

    int bodyY = CONTENT_BODY_Y + 4;
    int bodyH = CONTENT_BODY_H - 8;
    d.fillRect(0, CONTENT_BODY_Y, SCREEN_W, CONTENT_BODY_H, COL_PANEL);

    if (!p.valid) {
        d.setTextColor(COL_DIM, COL_PANEL);
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.drawString(String("No data: ") + Propagation::status(),
                     SCREEN_W / 2, bodyY + 30);
        d.setTextColor(COL_MUTED, COL_PANEL);
        d.drawString("Tap Refresh after WiFi connects",
                     SCREEN_W / 2, bodyY + 50);
        return;
    }

    // Two rows of 4 metric cards.
    int margin = 6;
    int gap = 4;
    int colW = (SCREEN_W - margin * 2 - gap * 3) / 4;
    int cellH = 38;
    int row1Y = bodyY;
    int row2Y = row1Y + cellH + gap;

    auto cellX = [&](int i) { return margin + i * (colW + gap); };

    drawMetricCell(cellX(0), row1Y, colW, cellH, "SFI",   p.solarFlux);
    drawMetricCell(cellX(1), row1Y, colW, cellH, "SN",    p.sunspots);
    drawMetricCell(cellX(2), row1Y, colW, cellH, "A-IDX", p.aIndex);
    drawMetricCell(cellX(3), row1Y, colW, cellH, "K-IDX", p.kIndex);

    drawMetricCell(cellX(0), row2Y, colW, cellH, "X-RAY", p.xrayClass);
    drawMetricCell(cellX(1), row2Y, colW, cellH, "S/N",   p.signalNoise);
    drawMetricCell(cellX(2), row2Y, colW, cellH, "MUF",   p.muf);
    drawMetricCell(cellX(3), row2Y, colW, cellH, "AURORA",p.aurora);

    // Band conditions card.
    int bandsY = row2Y + cellH + gap + 2;
    int bandsH = bodyY + bodyH - bandsY;
    drawCard(margin, bandsY, SCREEN_W - margin * 2, bandsH);

    int hdrY = bandsY + 4;
    d.setFont(&fonts::Font0);
    d.setTextColor(COL_MUTED, COL_CARD);
    d.setTextDatum(top_left);
    d.drawString("BAND",  margin + 10,  hdrY);
    d.drawString("DAY",   margin + 110, hdrY);
    d.drawString("NIGHT", margin + 200, hdrY);

    int rowsY = hdrY + 14;
    int rowH = (bandsH - 18) / 4;
    if (rowH < 14) rowH = 14;
    d.setFont(&fonts::Font2);
    for (size_t i = 0; i < p.bands.size() && i < 4; i++) {
        const auto& b = p.bands[i];
        int ry = rowsY + (int)i * rowH;
        d.setTextColor(COL_FG, COL_CARD);
        d.setTextDatum(middle_left);
        d.drawString(b.band, margin + 10, ry + rowH / 2);

        d.setTextColor(condColor(b.dayCond), COL_CARD);
        d.drawString(b.dayCond.length() ? b.dayCond : String("-"),  margin + 110, ry + rowH / 2);
        d.setTextColor(condColor(b.nightCond), COL_CARD);
        d.drawString(b.nightCond.length() ? b.nightCond : String("-"), margin + 200, ry + rowH / 2);
    }
}

void touch(int x, int y) {
    if (s_refreshBtn.contains(x, y)) {
        Propagation::fetchNow();
        s_lastSig = "";
    }
}

}}
