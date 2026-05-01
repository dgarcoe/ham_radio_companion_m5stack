#pragma once

#include <Arduino.h>

namespace Ui {

enum class Screen {
    Launcher = 0,    // home with clock + tile grid
    DxCluster,
    Propagation,
    Alerts,
    Beacons,
    Pota,
    Bearing,
    Noaa,
    Contests,
    Grayline,
    Settings,
    Count
};

void begin();
void loop();

void setScreen(Screen s);
Screen currentScreen();
void goHome();   // shorthand for setScreen(Launcher)

// Mark the screen dirty so the next loop iteration runs a full redraw,
// including the chrome. Useful after a modal (keyboard / int editor)
// returns and has scribbled over the entire framebuffer.
void requestFullRedraw();

// Open the on-screen keyboard, modally, to edit a string value.
bool editString(const char* title, String* value, bool password = false, size_t maxLen = 48);

// Open a small numeric chooser (returns true on accept).
bool editInt(const char* title, int* value, int minV, int maxV);

}
