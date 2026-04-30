#pragma once

#include <Arduino.h>

namespace WifiMgr {
    void begin();
    void loop();
    bool isConnected();
    String ssid();
    String ip();
    int rssi();
    // Force a reconnect (after settings change).
    void reconnect();
}
