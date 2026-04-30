#include "propagation.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "wifi_manager.h"

namespace Propagation {

static PropagationData s_data;
static uint32_t s_lastFetchMs = 0;
static String s_status = "no data";

const PropagationData& data() { return s_data; }
String status() { return s_status; }

uint32_t lastFetchAgeSeconds() {
    if (!s_data.valid) return UINT32_MAX;
    return (millis() - s_lastFetchMs) / 1000UL;
}

// Tiny tag extractor, sufficient for the HamQSL XML feed.
static String tagValue(const String& xml, const String& tag, int from = 0, int* endOut = nullptr) {
    String openTag = "<" + tag;
    int s = xml.indexOf(openTag, from);
    if (s < 0) return "";
    int gt = xml.indexOf('>', s);
    if (gt < 0) return "";
    String closeTag = "</" + tag + ">";
    int e = xml.indexOf(closeTag, gt);
    if (e < 0) return "";
    if (endOut) *endOut = e + closeTag.length();
    String v = xml.substring(gt + 1, e);
    v.trim();
    return v;
}

static void parse(const String& xml) {
    PropagationData d;
    d.valid       = true;
    d.updated     = tagValue(xml, "updated");
    d.solarFlux   = tagValue(xml, "solarflux");
    d.aIndex      = tagValue(xml, "aindex");
    d.kIndex      = tagValue(xml, "kindex");
    d.sunspots    = tagValue(xml, "sunspots");
    d.xrayClass   = tagValue(xml, "xray");
    d.solarWind   = tagValue(xml, "solarwind");
    d.protonFlux  = tagValue(xml, "protonflux");
    d.electronFlux= tagValue(xml, "electonflux");  // note: spelled this way in the feed
    if (d.electronFlux.isEmpty()) d.electronFlux = tagValue(xml, "electronflux");
    d.aurora      = tagValue(xml, "aurora");
    d.muf         = tagValue(xml, "muf");
    d.hf          = tagValue(xml, "hf");
    d.signalNoise = tagValue(xml, "signalnoise");

    // Bands look like:
    //   <band name="80m-40m" time="day">Good</band>
    //   <band name="80m-40m" time="night">Good</band>
    int idx = 0;
    std::map<String, BandCondition> map;
    while (true) {
        int s = xml.indexOf("<band ", idx);
        if (s < 0) break;
        int gt = xml.indexOf('>', s);
        if (gt < 0) break;
        int e = xml.indexOf("</band>", gt);
        if (e < 0) break;
        String attrs = xml.substring(s + 6, gt);
        String value = xml.substring(gt + 1, e);
        value.trim();

        String name, when;
        int n1 = attrs.indexOf("name=\"");
        if (n1 >= 0) {
            int n2 = attrs.indexOf('"', n1 + 6);
            if (n2 > 0) name = attrs.substring(n1 + 6, n2);
        }
        int t1 = attrs.indexOf("time=\"");
        if (t1 >= 0) {
            int t2 = attrs.indexOf('"', t1 + 6);
            if (t2 > 0) when = attrs.substring(t1 + 6, t2);
        }

        BandCondition& bc = map[name];
        bc.band = name;
        if (when == "day")        bc.dayCond = value;
        else if (when == "night") bc.nightCond = value;

        idx = e + 7;
    }

    d.bands.clear();
    static const char* order[] = {"80m-40m", "30m-20m", "17m-15m", "12m-10m"};
    for (auto* key : order) {
        auto it = map.find(key);
        if (it != map.end()) d.bands.push_back(it->second);
    }
    for (auto& kv : map) {
        bool already = false;
        for (auto& b : d.bands) if (b.band == kv.first) { already = true; break; }
        if (!already) d.bands.push_back(kv.second);
    }

    s_data = d;
    s_lastFetchMs = millis();
}

// Fetch one URL, returning the response body or "" plus a status code via *codeOut.
// Sets *locationOut for redirect responses (301/302/303/307/308).
static String fetchOne(const String& url, int* codeOut, String* locationOut) {
    WiFiClient plain;
    WiFiClientSecure secure;
    secure.setInsecure(); // HamQSL etc.; we just need the data, not auth.

    HTTPClient http;
    http.setTimeout(10000);
    http.setUserAgent("M5HamCompanion/1.0");
    // We follow redirects manually so we can switch between HTTP and HTTPS clients.
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    const char* hdrs[] = { "Location" };
    http.collectHeaders(hdrs, 1);

    bool ok;
    if (url.startsWith("https://")) {
        ok = http.begin(secure, url);
    } else {
        ok = http.begin(plain, url);
    }
    if (!ok) {
        *codeOut = -1;
        return "";
    }

    int code = http.GET();
    *codeOut = code;
    if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308) {
        if (locationOut) *locationOut = http.header("Location");
        http.end();
        return "";
    }
    if (code != 200) {
        http.end();
        return "";
    }
    String body = http.getString();
    http.end();
    return body;
}

bool fetchNow() {
    if (!WifiMgr::isConnected()) { s_status = "no wifi"; return false; }
    auto& c = Config::get();
    if (c.propagationUrl.isEmpty()) { s_status = "no url"; return false; }

    String url = c.propagationUrl;
    String body;
    int code = 0;
    for (int hop = 0; hop < 4; hop++) {
        String location;
        body = fetchOne(url, &code, &location);
        if (code == 200 && body.length()) break;
        if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308) {
            if (location.isEmpty()) {
                s_status = String("redirect ") + code + " no Location";
                return false;
            }
            // Resolve scheme-relative redirects by inheriting from the previous URL.
            if (location.startsWith("//"))      location = (url.startsWith("https://") ? "https:" : "http:") + location;
            else if (location.startsWith("/")) {
                int schemeEnd = url.indexOf("://");
                int hostEnd = url.indexOf('/', schemeEnd >= 0 ? schemeEnd + 3 : 0);
                String origin = (hostEnd > 0) ? url.substring(0, hostEnd) : url;
                location = origin + location;
            }
            url = location;
            continue;
        }
        s_status = String("http ") + code;
        return false;
    }
    if (body.isEmpty()) {
        s_status = "no body";
        return false;
    }
    parse(body);
    s_status = "ok";
    return true;
}

void begin() {
    s_status = "no data";
}

void loop() {
    static uint32_t lastTry = 0;
    uint32_t now = millis();
    bool needFetch =
        (s_data.valid == false && WifiMgr::isConnected() && now - lastTry > 5000) ||
        (s_data.valid && now - s_lastFetchMs > 30UL * 60UL * 1000UL && now - lastTry > 60000);
    if (needFetch) {
        lastTry = now;
        fetchNow();
    }
}

}
