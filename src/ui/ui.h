#pragma once

#include <Arduino.h>

namespace Ui {

enum class Tab {
    Home = 0,
    DxCluster,
    Propagation,
    Alerts,
    Settings,
    Count
};

void begin();
void loop();

// Switch to a particular tab (also used by the keyboard's "back" action).
void setTab(Tab t);
Tab currentTab();

// Open the on-screen keyboard, modally, to edit a string value.
// Returns true if the user accepted; the new value is written to *value.
bool editString(const char* title, String* value, bool password = false, size_t maxLen = 48);

// Open a small numeric chooser (returns true on accept).
bool editInt(const char* title, int* value, int minV, int maxV);

}
