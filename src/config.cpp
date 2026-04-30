#include "config.h"

#include <Preferences.h>
#include <ArduinoJson.h>

AppConfig Config::_cfg;

AppConfig& Config::get() { return _cfg; }

static const char* NS = "hamcomp";
static const char* KEY = "cfg";

void Config::load() {
    Preferences p;
    p.begin(NS, true);
    String json = p.getString(KEY, "");
    p.end();

    if (json.isEmpty()) {
        // Seed with one example alert so the user sees the format.
        AlertRule a;
        a.name = "Example: 20m CW";
        a.band = "20m";
        a.mode = "CW";
        a.enabled = false;
        _cfg.alerts.push_back(a);
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, json)) return;

    _cfg.wifiSsid       = doc["wifiSsid"]       | _cfg.wifiSsid;
    _cfg.wifiPass       = doc["wifiPass"]       | _cfg.wifiPass;
    _cfg.myCallsign     = doc["myCallsign"]     | _cfg.myCallsign;
    _cfg.clusterHost    = doc["clusterHost"]    | _cfg.clusterHost;
    _cfg.clusterPort    = doc["clusterPort"]    | _cfg.clusterPort;
    _cfg.propagationUrl = doc["propagationUrl"] | _cfg.propagationUrl;
    _cfg.utcOffset      = doc["utcOffset"]      | _cfg.utcOffset;
    _cfg.soundEnabled   = doc["soundEnabled"]   | _cfg.soundEnabled;

    _cfg.alerts.clear();
    for (JsonVariant v : doc["alerts"].as<JsonArray>()) {
        AlertRule a;
        a.name      = v["name"]      | "";
        a.band      = v["band"]      | "any";
        a.mode      = v["mode"]      | "any";
        a.prefix    = v["prefix"]    | "";
        a.callMatch = v["callMatch"] | "";
        a.enabled   = v["enabled"]   | true;
        _cfg.alerts.push_back(a);
    }
}

void Config::save() {
    JsonDocument doc;
    doc["wifiSsid"]       = _cfg.wifiSsid;
    doc["wifiPass"]       = _cfg.wifiPass;
    doc["myCallsign"]     = _cfg.myCallsign;
    doc["clusterHost"]    = _cfg.clusterHost;
    doc["clusterPort"]    = _cfg.clusterPort;
    doc["propagationUrl"] = _cfg.propagationUrl;
    doc["utcOffset"]      = _cfg.utcOffset;
    doc["soundEnabled"]   = _cfg.soundEnabled;

    JsonArray arr = doc["alerts"].to<JsonArray>();
    for (auto& a : _cfg.alerts) {
        JsonObject o = arr.add<JsonObject>();
        o["name"]      = a.name;
        o["band"]      = a.band;
        o["mode"]      = a.mode;
        o["prefix"]    = a.prefix;
        o["callMatch"] = a.callMatch;
        o["enabled"]   = a.enabled;
    }

    String json;
    serializeJson(doc, json);

    Preferences p;
    p.begin(NS, false);
    p.putString(KEY, json);
    p.end();
}
