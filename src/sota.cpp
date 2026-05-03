#include "sota.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "wifi_manager.h"

namespace Sota {

static std::vector<SotaSpot> s_spots;
static uint32_t s_lastFetchMs = 0;
static String s_status = "no data";

static const size_t kMaxSpots     = 60;
static const uint32_t kRefreshMs  = 120UL * 1000UL;  // 2-min cadence
static const uint32_t kRetryMs    =  15UL * 1000UL;

const std::vector<SotaSpot>& spots() { return s_spots; }
String status() { return s_status; }
size_t spotCount() { return s_spots.size(); }

uint32_t lastFetchAgeSeconds() {
    if (s_lastFetchMs == 0) return UINT32_MAX;
    return (millis() - s_lastFetchMs) / 1000UL;
}

static String fetchOne(const String& url, int* codeOut, String* locationOut) {
    WiFiClientSecure secure;
    secure.setInsecure();
    WiFiClient plain;

    HTTPClient http;
    http.setTimeout(10000);
    http.setUserAgent("M5HamCompanion/1.0");
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    const char* hdrs[] = { "Location" };
    http.collectHeaders(hdrs, 1);

    bool ok = url.startsWith("https://")
        ? http.begin(secure, url)
        : http.begin(plain,  url);
    if (!ok) { *codeOut = -1; return ""; }

    int code = http.GET();
    *codeOut = code;
    if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308) {
        if (locationOut) *locationOut = http.header("Location");
        http.end();
        return "";
    }
    if (code != 200) { http.end(); return ""; }
    String body = http.getString();
    http.end();
    return body;
}

static String fetchWithRedirects(const String& startUrl) {
    String url = startUrl;
    for (int hop = 0; hop < 4; hop++) {
        int code = 0;
        String location;
        String body = fetchOne(url, &code, &location);
        if (code == 200 && body.length()) return body;
        if (code >= 301 && code <= 308) {
            if (location.isEmpty()) { s_status = String("redirect ") + code; return ""; }
            if (location.startsWith("//")) {
                location = (url.startsWith("https://") ? "https:" : "http:") + location;
            } else if (location.startsWith("/")) {
                int se = url.indexOf("://");
                int he = url.indexOf('/', se >= 0 ? se + 3 : 0);
                String origin = (he > 0) ? url.substring(0, he) : url;
                location = origin + location;
            }
            url = location;
            continue;
        }
        s_status = String("http ") + code;
        return "";
    }
    s_status = "too many redirects";
    return "";
}

bool fetchNow() {
    if (!WifiMgr::isConnected()) { s_status = "no wifi"; return false; }

    String body = fetchWithRedirects("https://api2.sota.org.uk/api/spots/60/all");
    if (body.isEmpty()) return false;

    JsonDocument filter;
    filter[0]["activatorCallsign"] = true;
    filter[0]["summitCode"]        = true;
    filter[0]["summitDetails"]     = true;
    filter[0]["frequency"]         = true;
    filter[0]["mode"]              = true;
    filter[0]["comments"]          = true;
    filter[0]["timeStamp"]         = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body,
                                               DeserializationOption::Filter(filter));
    if (err) {
        s_status = String("json: ") + err.c_str();
        return false;
    }

    s_spots.clear();
    s_spots.reserve(kMaxSpots);
    for (JsonVariant v : doc.as<JsonArray>()) {
        if (s_spots.size() >= kMaxSpots) break;
        SotaSpot sp;
        sp.activator   = (const char*)(v["activatorCallsign"] | "");
        sp.summit      = (const char*)(v["summitCode"]        | "");
        sp.summitName  = (const char*)(v["summitDetails"]     | "");
        sp.mode        = (const char*)(v["mode"]              | "");
        sp.comments    = (const char*)(v["comments"]          | "");
        sp.timeUtc     = (const char*)(v["timeStamp"]         | "");

        // Frequency comes as a string; the SOTA API uses MHz ("14.285") or
        // occasionally kHz ("14285"). Heuristic: values < 1000 are MHz.
        const char* freqStr = v["frequency"] | "0";
        float f = String(freqStr).toFloat();
        sp.freqKHz = (f > 0 && f < 1000.0f) ? f * 1000.0f : f;

        sp.rxMillis = millis();
        s_spots.push_back(std::move(sp));
    }

    s_lastFetchMs = millis();
    s_status = "ok";
    return true;
}

void begin() {
    s_status = "no data";
}

void loop() {
    static uint32_t lastTry = 0;
    uint32_t now = millis();
    if (!WifiMgr::isConnected()) return;

    bool needFetch =
        (s_lastFetchMs == 0 && now - lastTry > kRetryMs) ||
        (s_lastFetchMs > 0  && now - s_lastFetchMs > kRefreshMs && now - lastTry > kRetryMs);
    if (needFetch) {
        lastTry = now;
        fetchNow();
    }
}

}
