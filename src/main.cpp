#include <M5Unified.h>
#include <Arduino.h>

#include "config.h"
#include "wifi_manager.h"
#include "dx_cluster.h"
#include "propagation.h"
#include "alerts.h"
#include "pota.h"
#include "noaa.h"
#include "contests.h"
#include "satellites.h"
#include "ui/ui.h"

static void serialBanner() {
    auto& cfg = Config::get();
    Serial.println();
    Serial.println("=== M5Stack Core2 Ham Radio Companion ===");
    Serial.printf("Config   : loaded from %s\n", Config::loadSource().c_str());
    Serial.printf("Callsign : %s\n", cfg.myCallsign.c_str());
    Serial.printf("WiFi     : %s\n", cfg.wifiSsid.length() ? cfg.wifiSsid.c_str() : "(unset)");
    Serial.printf("Cluster  : %s:%u\n", cfg.clusterHost.c_str(), cfg.clusterPort);
    Serial.printf("Prop URL : %s\n", cfg.propagationUrl.c_str());
    Serial.println("Tap the Settings tile on the device home screen to configure.");
    Serial.println();
}

void setup() {
    auto cfg = M5.config();
    cfg.output_power = true;
    M5.begin(cfg);
    M5.Display.setBrightness(160);
    M5.Display.setRotation(1);

    Serial.begin(115200);

    Config::load();
    serialBanner();

    WifiMgr::begin();
    DxCluster::begin();
    Propagation::begin();
    Pota::begin();
    Noaa::begin();
    Contests::begin();
    Satellites::begin();
    Alerts::begin();
    Ui::begin();
}

void loop() {
    M5.update();

    WifiMgr::loop();
    DxCluster::loop();
    Propagation::loop();
    Pota::loop();
    Noaa::loop();
    Contests::loop();
    Satellites::loop();
    Alerts::loop();
    Ui::loop();

    delay(5);
}
