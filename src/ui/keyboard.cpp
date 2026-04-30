#include "ui_internal.h"

namespace Ui {

// On-screen keyboard for editing strings. Modal: spins until OK/Cancel.

namespace {

constexpr int KB_KEY_W = 30;
constexpr int KB_KEY_H = 28;
constexpr int KB_TOP   = 110;

struct KeyDef { const char* lower; const char* upper; };

// 10-column rows + a final row with space/shift/backspace/ok.
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
    d.fillRoundRect(x, y, w, h, 3, highlight ? COL_TAB_SEL : COL_TAB_BG);
    d.drawRoundRect(x, y, w, h, 3, COL_DIM);
    d.setTextColor(COL_FG, highlight ? COL_TAB_SEL : COL_TAB_BG);
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
    d.fillRect(0, KB_TOP - 4, SCREEN_W, SCREEN_H - (KB_TOP - 4), COL_BG);
    drawRow(s_shift ? row1U : row1L, KB_TOP);
    drawRow(s_shift ? row2U : row2L, KB_TOP + KB_KEY_H + 2);
    drawRow(s_shift ? row3U : row3L, KB_TOP + (KB_KEY_H + 2) * 2);
    drawRow(s_shift ? row4U : row4L, KB_TOP + (KB_KEY_H + 2) * 3);

    int yBot = KB_TOP + (KB_KEY_H + 2) * 4;
    drawKey(10,  yBot, 60, KB_KEY_H, s_shift ? "shift" : "Shift", s_shift);
    drawKey(74,  yBot, 130, KB_KEY_H, "space");
    drawKey(208, yBot, 50, KB_KEY_H, "del");
    drawKey(262, yBot, 50, KB_KEY_H, "OK");
}

void drawHeaderBox(const char* title, const String& value, bool password) {
    auto& d = M5.Display;
    d.fillRect(0, 0, SCREEN_W, 60, COL_BG);
    d.drawFastHLine(0, 60, SCREEN_W, COL_DIM);

    d.setTextColor(COL_ACCENT, COL_BG);
    d.setFont(&fonts::Font2);
    d.setTextDatum(top_left);
    d.drawString(title, 6, 4);

    String shown = password ? String('*' /*placeholder*/) : value;
    if (password) {
        shown = "";
        for (size_t i = 0; i < value.length(); i++) shown += '*';
    }
    d.fillRect(6, 24, SCREEN_W - 12, 32, COL_TAB_BG);
    d.drawRect(6, 24, SCREEN_W - 12, 32, COL_DIM);
    d.setTextColor(COL_FG, COL_TAB_BG);
    d.setFont(&fonts::Font4);
    d.setTextDatum(middle_left);
    d.drawString(shown, 12, 40);

    d.setTextColor(COL_DIM, COL_BG);
    d.setFont(&fonts::Font2);
    d.drawString("Cancel", SCREEN_W - 60, 4);
}

// Returns the character pressed (or 0 if none). Special codes:
//   1 = shift, 2 = space, 8 = backspace, 10 = OK, 27 = cancel.
int hitTest(int x, int y) {
    if (y < 60 && x > SCREEN_W - 70) return 27; // cancel area top-right
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

    auto redraw = [&](){
        d.fillScreen(COL_BG);
        d.setTextColor(COL_ACCENT, COL_BG);
        d.setFont(&fonts::Font2);
        d.setTextDatum(top_left);
        d.drawString(title, 6, 4);

        d.setTextColor(COL_FG, COL_BG);
        d.setFont(&fonts::Font7);
        d.setTextDatum(middle_center);
        d.drawString(String(v), SCREEN_W / 2, 100);

        // Buttons: -10, -1, +1, +10 in a row, then OK / Cancel
        int bh = 36;
        auto btn = [&](int bx, int by, int bw, const char* label, uint16_t bg) {
            d.fillRoundRect(bx, by, bw, bh, 4, bg);
            d.setTextColor(COL_FG, bg);
            d.setFont(&fonts::Font2);
            d.setTextDatum(middle_center);
            d.drawString(label, bx + bw / 2, by + bh / 2);
        };
        btn(6,   170, 60, "-10", COL_TAB_BG);
        btn(70,  170, 60, "-1",  COL_TAB_BG);
        btn(134, 170, 60, "+1",  COL_TAB_BG);
        btn(198, 170, 60, "+10", COL_TAB_BG);

        btn(80,  170 + bh + 8, 80, "Cancel", COL_BAD);
        btn(170, 170 + bh + 8, 80, "OK",     COL_OK);
    };
    redraw();

    while (true) {
        M5.update();
        auto t = M5.Touch.getDetail();
        if (t.wasPressed()) {
            int x = t.x, y = t.y;
            int by = 170, bh = 36;
            if (y >= by && y <= by + bh) {
                if      (x < 66)  v -= 10;
                else if (x < 130) v -= 1;
                else if (x < 194) v += 1;
                else              v += 10;
                if (v < minV) v = minV;
                if (v > maxV) v = maxV;
                redraw();
                continue;
            }
            int by2 = by + bh + 8;
            if (y >= by2 && y <= by2 + bh) {
                if (x >= 80 && x <= 160) return false;
                if (x >= 170 && x <= 250) { *value = v; return true; }
            }
        }
        delay(10);
    }
}

}
