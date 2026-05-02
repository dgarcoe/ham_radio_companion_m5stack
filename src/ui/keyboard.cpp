#include "ui_internal.h"

namespace Ui {

// Modal on-screen keyboard for editing strings.

namespace {

constexpr int KB_KEY_W = 30;
constexpr int KB_KEY_H = 24;    // was 28 — shorter keys keep last row on-screen
constexpr int KB_TOP   = 112;   // was 116 — shift up slightly to give 4px margin

static const char* row1L = "1234567890";
static const char* row1U = "!@#$%^&*()";
static const char* row2L = "qwertyuiop";
static const char* row2U = "QWERTYUIOP";
static const char* row3L = "asdfghjkl-";
static const char* row3U = "ASDFGHJKL_";
static const char* row4L = "zxcvbnm./?";
static const char* row4U = "ZXCVBNM,:/";

bool s_shift = false;

void drawKey(int x, int y, int w, int h, const String& label, bool highlight = false) {
    auto& d = M5.Display;
    uint16_t bg = highlight ? COL_ACCENT_D : COL_CARD;
    d.fillRoundRect(x, y, w, h, 4, bg);
    d.drawRoundRect(x, y, w, h, 4, COL_BORDER);
    d.setTextColor(COL_FG, bg);
    d.setTextDatum(middle_center);
    d.setFont(&fonts::Font2);
    d.drawString(label, x + w / 2, y + h / 2);
}

void drawRow(const char* row, int rowY) {
    for (int i = 0; i < 10; i++) {
        char c = row[i];
        char buf[2] = { c, 0 };
        drawKey(i * KB_KEY_W + 10, rowY, KB_KEY_W - 2, KB_KEY_H, String(buf));
    }
}

void drawKeyboard() {
    auto& d = M5.Display;
    d.fillRect(0, KB_TOP - 4, SCREEN_W, SCREEN_H - (KB_TOP - 4), COL_PANEL);
    drawRow(s_shift ? row1U : row1L, KB_TOP);
    drawRow(s_shift ? row2U : row2L, KB_TOP + KB_KEY_H + 2);
    drawRow(s_shift ? row3U : row3L, KB_TOP + (KB_KEY_H + 2) * 2);
    drawRow(s_shift ? row4U : row4L, KB_TOP + (KB_KEY_H + 2) * 3);

    int yBot = KB_TOP + (KB_KEY_H + 2) * 4;
    drawKey(10,  yBot, 60,  KB_KEY_H, s_shift ? "shift" : "Shift", s_shift);
    drawKey(74,  yBot, 130, KB_KEY_H, "space");
    drawKey(208, yBot, 50,  KB_KEY_H, "del");
    drawKey(262, yBot, 50,  KB_KEY_H, "OK");
}

void drawHeaderBox(const char* title, const String& value, bool password) {
    auto& d = M5.Display;
    d.fillRect(0, 0, SCREEN_W, KB_TOP - 4, COL_BG);

    // Title strip.
    d.setTextColor(COL_ACCENT, COL_BG);
    d.setFont(&fonts::Font2);
    d.setTextDatum(top_left);
    d.drawString(title, 10, 6);

    // Cancel hint top-right.
    d.setTextColor(COL_DIM, COL_BG);
    d.drawString("Cancel", SCREEN_W - 60, 6);

    // Input card.
    int boxX = 8, boxY = 28, boxW = SCREEN_W - 16, boxH = 56;
    d.fillRoundRect(boxX, boxY, boxW, boxH, 6, COL_CARD);
    d.drawRoundRect(boxX, boxY, boxW, boxH, 6, COL_BORDER);

    String shown;
    if (password) for (size_t i = 0; i < value.length(); i++) shown += '*';
    else          shown = value;

    d.setTextColor(COL_FG, COL_CARD);
    d.setFont(&fonts::Font4);
    d.setTextDatum(middle_left);
    d.drawString(shown, boxX + 10, boxY + boxH / 2);

    // Caret hint.
    d.setTextColor(COL_ACCENT, COL_CARD);
    d.drawString("|", boxX + 10 + d.textWidth(shown) + 2, boxY + boxH / 2);
}

// Returns the character pressed (or 0 if none). Special codes:
//   1 = shift, 2 = space, 8 = backspace, 10 = OK, 27 = cancel.
int hitTest(int x, int y) {
    if (y < 28 && x > SCREEN_W - 70) return 27; // cancel area top-right
    if (y < KB_TOP) return 0;
    int yBot = KB_TOP + (KB_KEY_H + 2) * 4;

    if (y >= yBot && y <= yBot + KB_KEY_H) {
        if (x >= 10  && x <= 70)  return 1;
        if (x >= 74  && x <= 204) return 2;
        if (x >= 208 && x <= 258) return 8;
        if (x >= 262 && x <= 312) return 10;
        return 0;
    }

    int rowIdx = (y - KB_TOP) / (KB_KEY_H + 2);
    int yInRow = (y - KB_TOP) % (KB_KEY_H + 2);
    if (yInRow > KB_KEY_H) return 0;
    if (rowIdx < 0 || rowIdx > 3) return 0;
    int col = (x - 10) / KB_KEY_W;
    if (col < 0 || col > 9) return 0;

    const char* row = nullptr;
    switch (rowIdx) {
        case 0: row = s_shift ? row1U : row1L; break;
        case 1: row = s_shift ? row2U : row2L; break;
        case 2: row = s_shift ? row3U : row3L; break;
        case 3: row = s_shift ? row4U : row4L; break;
    }
    return row ? row[col] : 0;
}

} // anon

bool editString(const char* title, String* value, bool password, size_t maxLen) {
    if (!value) return false;
    String buf = *value;
    s_shift = false;

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeaderBox(title, buf, password);
    drawKeyboard();

    uint32_t lastTouchMs = 0;
    while (true) {
        M5.update();
        auto t = M5.Touch.getDetail();
        if (t.wasPressed()) {
            uint32_t now = millis();
            if (now - lastTouchMs < 120) continue;
            lastTouchMs = now;
            int code = hitTest(t.x, t.y);
            if (code == 0) continue;
            if (code == 27) return false;
            if (code == 10) { *value = buf; return true; }
            if (code == 8)  { if (buf.length()) buf.remove(buf.length() - 1); }
            else if (code == 2) { if (buf.length() < maxLen) buf += ' '; }
            else if (code == 1) { s_shift = !s_shift; drawKeyboard(); continue; }
            else if (code >= 32 && code < 127) {
                if (buf.length() < maxLen) buf += (char)code;
                if (s_shift) { s_shift = false; drawKeyboard(); }
            }
            drawHeaderBox(title, buf, password);
        }
        delay(10);
    }
}

bool editInt(const char* title, int* value, int minV, int maxV) {
    if (!value) return false;
    int v = *value;
    auto& d = M5.Display;

    auto redraw = [&]() {
        d.fillScreen(COL_BG);

        // Title.
        d.setTextColor(COL_ACCENT, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(top_left);
        d.drawString(title, 10, 8);

        // Value card.
        d.fillRoundRect(40, 36, SCREEN_W - 80, 70, 8, COL_CARD);
        d.drawRoundRect(40, 36, SCREEN_W - 80, 70, 8, COL_BORDER);
        d.setTextColor(COL_FG, COL_CARD);
        d.setFont(&fonts::Font7);
        d.setTextDatum(middle_center);
        d.drawString(String(v), SCREEN_W / 2, 71);

        // Step buttons.
        int by = 120, bw = 60, bh = 32, gap = 8;
        int totalW = bw * 4 + gap * 3;
        int xStart = (SCREEN_W - totalW) / 2;
        const char* labels[] = { "-10", "-1", "+1", "+10" };
        for (int i = 0; i < 4; i++) {
            int bx = xStart + i * (bw + gap);
            d.fillRoundRect(bx, by, bw, bh, 5, COL_CARD);
            d.drawRoundRect(bx, by, bw, bh, 5, COL_BORDER);
            d.setTextColor(COL_FG, COL_CARD);
            d.setTextDatum(middle_center);
            d.setFont(&fonts::Font2);
            d.drawString(labels[i], bx + bw / 2, by + bh / 2);
        }

        // Cancel / OK.
        int by2 = 170, bw2 = 110, bh2 = 36;
        int xCancel = 24, xOk = SCREEN_W - bw2 - 24;
        d.fillRoundRect(xCancel, by2, bw2, bh2, 6, COL_BAD);
        d.drawRoundRect(xCancel, by2, bw2, bh2, 6, COL_BORDER);
        d.setTextColor(COL_FG, COL_BAD);
        d.setTextDatum(middle_center);
        d.drawString("Cancel", xCancel + bw2 / 2, by2 + bh2 / 2);

        d.fillRoundRect(xOk, by2, bw2, bh2, 6, COL_OK);
        d.drawRoundRect(xOk, by2, bw2, bh2, 6, COL_BORDER);
        d.setTextColor(COL_BG, COL_OK);
        d.drawString("OK", xOk + bw2 / 2, by2 + bh2 / 2);
    };
    redraw();

    while (true) {
        M5.update();
        auto t = M5.Touch.getDetail();
        if (t.wasPressed()) {
            int x = t.x, y = t.y;

            int by = 120, bw = 60, bh = 32, gap = 8;
            int totalW = bw * 4 + gap * 3;
            int xStart = (SCREEN_W - totalW) / 2;
            if (y >= by && y <= by + bh) {
                for (int i = 0; i < 4; i++) {
                    int bx = xStart + i * (bw + gap);
                    if (x >= bx && x <= bx + bw) {
                        if (i == 0) v -= 10;
                        if (i == 1) v -= 1;
                        if (i == 2) v += 1;
                        if (i == 3) v += 10;
                        if (v < minV) v = minV;
                        if (v > maxV) v = maxV;
                        redraw();
                        break;
                    }
                }
                continue;
            }

            int by2 = 170, bw2 = 110, bh2 = 36;
            int xCancel = 24, xOk = SCREEN_W - bw2 - 24;
            if (y >= by2 && y <= by2 + bh2) {
                if (x >= xCancel && x <= xCancel + bw2) return false;
                if (x >= xOk     && x <= xOk     + bw2) { *value = v; return true; }
            }
        }
        delay(10);
    }
}

}
