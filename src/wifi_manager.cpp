#include "wifi_manager.h"

#include <WiFi.h>
#include <time.h>
#include "config.h"

namespace WifiMgr {

static uint32_t s_lastAttempt = 0;
static bool s_timeSynced = false;

void begin() {
    WiFi.mode(WIFI_STA);
    auto& c = Config::get();
    if (c.wifiSsid.length()) {
        WiFi.begin(c.wifiSsid.c_str(), c.wifiPass.c_str());
    }
}

void reconnect() {
    WiFi.disconnect(true, true);
    s_timeSynced = false;
    s_lastAttempt = 0;
    begin();
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!s_timeSynced) {
            // UTC. We display offset locally where needed.
            configTime(0, 0, "pool.ntp.org", "time.nist.gov");
            s_timeSynced = true;
        }
        return;
    }

    uint32_t now = millis();
    if (now - s_lastAttempt < 15000) return;
    s_lastAttempt = now;

    auto& c = Config::get();
    if (!c.wifiSsid.length()) return;
    WiFi.disconnect();
    WiFi.begin(c.wifiSsid.c_str(), c.wifiPass.c_str());
}

bool isConnected() { return WiFi.status() == WL_CONNECTED; }
String ssid()      { return WiFi.SSID(); }
String ip()        { return WiFi.localIP().toString(); }
int rssi()         { return WiFi.RSSI(); }

}
