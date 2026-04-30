#include "config.h"

#include <Preferences.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

AppConfig Config::_cfg;
bool Config::_fromFile = false;
bool Config::_fromNvs  = false;

AppConfig& Config::get()       { return _cfg; }
bool Config::loadedFromFile()  { return _fromFile; }
bool Config::loadedFromNvs()   { return _fromNvs; }
String Config::loadSource()    {
    if (_fromFile) return "file";
    if (_fromNvs)  return "nvs";
    return "defaults";
}

static const char* NS         = "hamcomp";
static const char* KEY        = "cfg";
static const char* FS_PATH    = "/config.json";

static bool s_fsMounted = false;

static bool ensureFs() {
    if (s_fsMounted) return true;
    // Try mount; if FS is uninitialized, format-on-fail.
    if (LittleFS.begin(true)) { s_fsMounted = true; return true; }
    return false;
}

static void applyJson(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;

    AppConfig& c = Config::get();
    c.wifiSsid       = doc["wifiSsid"]       | c.wifiSsid;
    c.wifiPass       = doc["wifiPass"]       | c.wifiPass;
    c.myCallsign     = doc["myCallsign"]     | c.myCallsign;
    c.clusterHost    = doc["clusterHost"]    | c.clusterHost;
    c.clusterPort    = doc["clusterPort"]    | c.clusterPort;
    c.propagationUrl = doc["propagationUrl"] | c.propagationUrl;
    c.utcOffset      = doc["utcOffset"]      | c.utcOffset;
    c.soundEnabled   = doc["soundEnabled"]   | c.soundEnabled;

    if (doc["alerts"].is<JsonArray>()) {
        c.alerts.clear();
        for (JsonVariant v : doc["alerts"].as<JsonArray>()) {
            AlertRule a;
            a.name      = v["name"]      | "";
            a.band      = v["band"]      | "any";
            a.mode      = v["mode"]      | "any";
            a.prefix    = v["prefix"]    | "";
            a.callMatch = v["callMatch"] | "";
            a.enabled   = v["enabled"]   | true;
            c.alerts.push_back(a);
        }
    }
}

static String currentJson() {
    JsonDocument doc;
    AppConfig& c = Config::get();
    doc["wifiSsid"]       = c.wifiSsid;
    doc["wifiPass"]       = c.wifiPass;
    doc["myCallsign"]     = c.myCallsign;
    doc["clusterHost"]    = c.clusterHost;
    doc["clusterPort"]    = c.clusterPort;
    doc["propagationUrl"] = c.propagationUrl;
    doc["utcOffset"]      = c.utcOffset;
    doc["soundEnabled"]   = c.soundEnabled;

    JsonArray arr = doc["alerts"].to<JsonArray>();
    for (auto& a : c.alerts) {
        JsonObject o = arr.add<JsonObject>();
        o["name"]      = a.name;
        o["band"]      = a.band;
        o["mode"]      = a.mode;
        o["prefix"]    = a.prefix;
        o["callMatch"] = a.callMatch;
        o["enabled"]   = a.enabled;
    }
    String json;
    serializeJsonPretty(doc, json);
    return json;
}

static String readFs() {
    if (!ensureFs()) return "";
    if (!LittleFS.exists(FS_PATH)) return "";
    File f = LittleFS.open(FS_PATH, "r");
    if (!f) return "";
    String s = f.readString();
    f.close();
    return s;
}

static bool writeFs(const String& json) {
    if (!ensureFs()) return false;
    File f = LittleFS.open(FS_PATH, "w");
    if (!f) return false;
    size_t n = f.print(json);
    f.close();
    return n > 0;
}

static String readNvs() {
    Preferences p;
    p.begin(NS, true);
    String s = p.getString(KEY, "");
    p.end();
    return s;
}

static void writeNvs(const String& json) {
    Preferences p;
    p.begin(NS, false);
    p.putString(KEY, json);
    p.end();
}

void Config::load() {
    _fromFile = false;
    _fromNvs  = false;

    // Try LittleFS first - this is the user-supplied config file.
    String json = readFs();
    if (!json.isEmpty()) {
        applyJson(json);
        _fromFile = true;
        return;
    }

    // Fall back to whatever the user previously saved on-device.
    json = readNvs();
    if (!json.isEmpty()) {
        applyJson(json);
        _fromNvs = true;
        return;
    }

    // Built-in defaults: seed one disabled example alert so the rule shape is
    // visible in the Alerts tab.
    AlertRule a;
    a.name = "Example: 20m CW";
    a.band = "20m";
    a.mode = "CW";
    a.enabled = false;
    _cfg.alerts.push_back(a);
}

void Config::save() {
    String json = currentJson();
    writeNvs(json);
    // Best-effort mirror to LittleFS so a `pio run -t downloadfs` can fetch
    // the live config back. Failures (e.g. FS not mounted) are non-fatal.
    writeFs(json);
}
