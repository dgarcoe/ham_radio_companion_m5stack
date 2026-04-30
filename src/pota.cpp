#include "pota.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "wifi_manager.h"

namespace Pota {

static std::vector<PotaSpot> s_spots;
static uint32_t s_lastFetchMs = 0;
static String s_status = "no data";

// The list can get large (~hundreds). Cap to avoid hogging memory.
static const size_t kMaxSpots = 80;
// Refresh cadence once we've succeeded once. POTA spots churn quickly.
static const uint32_t kRefreshMs = 60UL * 1000UL;
// Backoff for failures.
static const uint32_t kRetryMs   = 15UL * 1000UL;

const std::vector<PotaSpot>& spots() { return s_spots; }
String status() { return s_status; }
size_t spotCount() { return s_spots.size(); }

uint32_t lastFetchAgeSeconds() {
    if (s_lastFetchMs == 0) return UINT32_MAX;
    return (millis() - s_lastFetchMs) / 1000UL;
}

static String fetchOne(const String& url, int* codeOut, String* locationOut) {
    WiFiClient plain;
    WiFiClientSecure secure;
    secure.setInsecure();

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
        if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308) {
            if (location.isEmpty()) { s_status = String("redirect ") + code + " no Location"; return ""; }
            if (location.startsWith("//")) {
                location = (url.startsWith("https://") ? "https:" : "http:") + location;
            } else if (location.startsWith("/")) {
                int schemeEnd = url.indexOf("://");
                int hostEnd = url.indexOf('/', schemeEnd >= 0 ? schemeEnd + 3 : 0);
                String origin = (hostEnd > 0) ? url.substring(0, hostEnd) : url;
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

    String body = fetchWithRedirects("https://api.pota.app/spot/activator");
    if (body.isEmpty()) return false;

    // The POTA endpoint can return a few hundred KB; size the document
    // accordingly. ArduinoJson v7 supports streaming, but we already have the
    // body in a String, so a generous filter keeps RAM in check.
    JsonDocument filter;
    filter[0]["activator"]    = true;
    filter[0]["reference"]    = true;
    filter[0]["name"]         = true;
    filter[0]["locationDesc"] = true;
    filter[0]["frequency"]    = true;
    filter[0]["mode"]         = true;
    filter[0]["spotter"]      = true;
    filter[0]["comments"]     = true;
    filter[0]["spotTime"]     = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body, DeserializationOption::Filter(filter));
    if (err) {
        s_status = String("json: ") + err.c_str();
        return false;
    }

    s_spots.clear();
    s_spots.reserve(kMaxSpots);
    for (JsonVariant v : doc.as<JsonArray>()) {
        if (s_spots.size() >= kMaxSpots) break;
        PotaSpot sp;
        sp.activator = (const char*)(v["activator"]  | "");
        sp.reference = (const char*)(v["reference"]  | "");
        sp.parkName  = (const char*)(v["name"]       | "");
        sp.location  = (const char*)(v["locationDesc"] | "");
        const char* freqStr = v["frequency"] | "0";
        sp.freqKHz = String(freqStr).toFloat();
        sp.mode      = (const char*)(v["mode"]       | "");
        sp.spotter   = (const char*)(v["spotter"]    | "");
        sp.comments  = (const char*)(v["comments"]   | "");
        sp.spotTime  = (const char*)(v["spotTime"]   | "");
        sp.rxMillis  = millis();
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
