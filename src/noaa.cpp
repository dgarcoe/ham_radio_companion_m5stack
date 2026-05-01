#include "noaa.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "wifi_manager.h"

namespace Noaa {

static std::vector<NoaaAlert> s_alerts;
static uint32_t s_lastFetchMs = 0;
static String s_status = "no data";

static const size_t kMaxAlerts = 30;
static const uint32_t kRefreshMs = 15UL * 60UL * 1000UL;   // 15 minutes
static const uint32_t kRetryMs   = 30UL * 1000UL;

const std::vector<NoaaAlert>& alerts() { return s_alerts; }
String status() { return s_status; }
size_t count() { return s_alerts.size(); }

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
            if (location.isEmpty()) { s_status = String("redirect ") + code; return ""; }
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

// Pull the first non-empty content line out of an SWPC message body. The
// messages start with header lines like "Space Weather Message Code: ALTK04 /
// Issue Time: ...", followed by a blank line, then the actual narrative.
static String summarise(const String& message) {
    int i = 0;
    while (i < (int)message.length()) {
        int nl = message.indexOf('\n', i);
        if (nl < 0) nl = message.length();
        String line = message.substring(i, nl);
        line.trim();
        // Skip header lines (key: value) and blank lines.
        if (line.length() && line.indexOf(": ") < 0) {
            // Trim to a digestible length for the list cards.
            if (line.length() > 80) line = line.substring(0, 77) + "...";
            return line;
        }
        i = nl + 1;
    }
    return "";
}

bool fetchNow() {
    if (!WifiMgr::isConnected()) { s_status = "no wifi"; return false; }

    String body = fetchWithRedirects("https://services.swpc.noaa.gov/products/alerts.json");
    if (body.isEmpty()) return false;

    // The feed often returns ~200 alerts. Keep only the newest few - the API
    // already orders them newest-first.
    JsonDocument filter;
    filter[0]["product_id"]   = true;
    filter[0]["issue_datetime"] = true;
    filter[0]["message"]      = true;

    JsonDocument doc;
    DeserializationError err =
        deserializeJson(doc, body, DeserializationOption::Filter(filter));
    if (err) {
        s_status = String("json: ") + err.c_str();
        return false;
    }

    s_alerts.clear();
    for (JsonVariant v : doc.as<JsonArray>()) {
        if (s_alerts.size() >= kMaxAlerts) break;
        NoaaAlert a;
        a.issued  = (const char*)(v["issue_datetime"] | "");
        a.code    = (const char*)(v["product_id"]    | "");
        a.message = (const char*)(v["message"]       | "");
        a.summary = summarise(a.message);
        s_alerts.push_back(std::move(a));
    }

    s_lastFetchMs = millis();
    s_status = "ok";
    return true;
}

void begin() { s_status = "no data"; }

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
