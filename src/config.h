#pragma once

#include <Arduino.h>
#include <cstdint>
#include <vector>

struct AlertRule {
    String name;
    String band;        // e.g. "20m", "40m", "any"
    String mode;        // e.g. "CW", "SSB", "FT8", "any"
    String prefix;      // DXCC prefix prefix-match, e.g. "VK", "JA", "" for any
    String callMatch;   // substring match on spotted callsign
    bool enabled = true;
};

struct AppConfig {
    // WiFi
    String wifiSsid;
    String wifiPass;

    // Identity
    String myCallsign = "N0CALL";
    String myGrid     = "";        // Maidenhead, e.g. "FN30as" - your QTH

    // DX cluster
    String clusterHost = "dxc.k0xm.net";
    uint16_t clusterPort = 7300;

    // Propagation source
    String propagationUrl = "https://www.hamqsl.com/solarxml.php";

    // Misc
    int8_t utcOffset = 0;     // hours offset for local time display
    bool soundEnabled = true; // audible alerts

    // DX list filters (UI only - alerts use their own per-rule filters).
    String filterBand = "any";   // "any", "160m", ..., "2m"
    String filterMode = "any";   // "any", "CW", "SSB", "FT8", "RTTY", "DIGI"
    String filterCont = "any";   // "any" or "my" - filter spotter continent

    // Alerts
    std::vector<AlertRule> alerts;

    // Bitmask of launcher tiles to hide. Bit N corresponds to the Nth tile
    // declared in screen_launcher.cpp (DX=0, Prop=1, ...). 0 = all visible.
    uint32_t tileHideMask = 0;
};

class Config {
public:
    static AppConfig& get();

    // Load order: LittleFS /config.json (if present) -> NVS -> built-in defaults.
    // Sets `loadedFromFile` / `loadedFromNvs` so the UI can show where the
    // current settings came from.
    static void load();
    static void save();           // persists to NVS (and to LittleFS too if mounted)

    static bool loadedFromFile();
    static bool loadedFromNvs();
    static String loadSource();   // "file" / "nvs" / "defaults"

private:
    static AppConfig _cfg;
    static bool _fromFile;
    static bool _fromNvs;
};
