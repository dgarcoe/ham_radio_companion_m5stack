#include "satellites.h"

#include <algorithm>

#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "geo.h"
#include "sat_sgp4.h"
#include "wifi_manager.h"

namespace Satellites {

// Curated list of popular amateur / human-spaceflight satellites. We fetch
// fresh TLEs for these from Celestrak; if the fetch fails we fall back to
// whatever's cached on LittleFS from a previous run.
struct SatDef {
    int          norad;
    const char*  name;
};

static const SatDef kSatDefs[] = {
    { 25544, "ISS"     },
    { 43017, "AO-91"   },
    { 43137, "AO-92"   },
    { 44909, "RS-44"   },
    { 44829, "IO-117"  },
    { 39444, "AO-73"   },
    { 47438, "FO-99"   },
    { 51080, "GreenC1" },
};
static const int kSatCount = sizeof(kSatDefs) / sizeof(kSatDefs[0]);

static const char* kTlePath = "/tle.txt";

static const uint32_t kRefreshMs = 24UL * 3600UL * 1000UL;  // refresh TLEs once a day
static const uint32_t kRetryMs   = 30UL * 1000UL;
static const uint32_t kRecomputeMs = 5UL * 60UL * 1000UL;   // recompute passes every 5 min
static const int      kHorizonHours = 36;
static const float    kMinElev = 5.0f;                      // useful pass threshold

static std::vector<SatElset>   s_sats;
static std::vector<SatPass>    s_passes;
static String                  s_status = "no data";
static uint32_t                s_lastFetchMs = 0;
static uint32_t                s_lastComputeMs = 0;
static bool                    s_needRefresh = true;
static bool                    s_ready = false;

const std::vector<SatPass>& passes() { return s_passes; }
String status()                      { return s_status; }
bool ready()                         { return s_ready; }
size_t tleCount()                    { return s_sats.size(); }

uint32_t lastFetchAgeSeconds() {
    if (s_lastFetchMs == 0) return UINT32_MAX;
    return (millis() - s_lastFetchMs) / 1000UL;
}

void requestRefresh() { s_needRefresh = true; }

// --- TLE fetch ---------------------------------------------------------------

static String fetchUrl(const String& url) {
    WiFiClientSecure secure;
    secure.setInsecure();

    HTTPClient http;
    http.setTimeout(10000);
    http.setUserAgent("M5HamCompanion/1.0");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(secure, url)) return "";
    int code = http.GET();
    if (code != 200) { http.end(); return ""; }
    String body = http.getString();
    http.end();
    return body;
}

// Celestrak's "amateur" group is the most reliable single feed that contains
// our entire list. ~50 KB, but parsing it streamingly keeps memory bounded
// since we only keep TLEs for satellites in kSatDefs.
static bool fetchTLEs(String& outBlob) {
    String body = fetchUrl("https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle");
    if (body.isEmpty()) {
        body = fetchUrl("https://celestrak.org/NORAD/elements/gp.php?GROUP=stations&FORMAT=tle");
        if (body.isEmpty()) return false;
    }
    outBlob = body;
    return true;
}

// Walk a 3-line-set TLE blob, picking out only the satellites we care about
// (matched by NORAD ID). Returns the concatenated 3-line records.
static String filterAndParse(const String& blob) {
    s_sats.clear();
    s_sats.reserve(kSatCount);
    String kept;

    int idx = 0;
    while (idx < (int)blob.length()) {
        int e1 = blob.indexOf('\n', idx); if (e1 < 0) break;
        int e2 = blob.indexOf('\n', e1 + 1); if (e2 < 0) break;
        int e3 = blob.indexOf('\n', e2 + 1); if (e3 < 0) e3 = blob.length();

        String name = blob.substring(idx, e1); name.trim();
        String l1   = blob.substring(e1 + 1, e2);
        String l2   = blob.substring(e2 + 1, e3);
        idx = e3 + 1;

        if (l1.length() < 69 || l2.length() < 69) continue;
        if (l1[0] != '1' || l2[0] != '2')        continue;

        // NORAD from line 1, cols 2-6
        int norad = atoi(l1.substring(2, 7).c_str());
        const SatDef* def = nullptr;
        for (int i = 0; i < kSatCount; i++) {
            if (kSatDefs[i].norad == norad) { def = &kSatDefs[i]; break; }
        }
        if (!def) continue;

        SatElset sat;
        if (!sgp4Init(&sat, def->name, l1.c_str(), l2.c_str())) continue;
        s_sats.push_back(sat);

        kept += def->name; kept += '\n';
        kept += l1;        kept += '\n';
        kept += l2;        kept += '\n';
    }
    return kept;
}

static bool loadFromFs() {
    if (!LittleFS.begin(true)) return false;
    if (!LittleFS.exists(kTlePath)) return false;
    File f = LittleFS.open(kTlePath, "r");
    if (!f) return false;
    String blob = f.readString();
    f.close();
    if (blob.isEmpty()) return false;
    filterAndParse(blob);
    return !s_sats.empty();
}

static void saveToFs(const String& blob) {
    if (!LittleFS.begin(true)) return;
    File f = LittleFS.open(kTlePath, "w");
    if (!f) return;
    f.print(blob);
    f.close();
}

// --- Pass computation --------------------------------------------------------

static bool computeOnePass(const SatElset& sat,
                           double obsLat, double obsLon,
                           time_t startT, time_t endT,
                           SatPass& out) {
    // Coarse scan in 60-second steps to find a rise above kMinElev.
    const int kStep = 60;
    bool inPass = false;
    time_t aos = 0, los = 0, maxT = 0;
    float aosAz = 0, losAz = 0, maxEl = 0;

    double prevEl = -90.0;
    double prevLat = 0, prevLon = 0;
    bool havePrev = false;

    for (time_t t = startT; t <= endT; t += kStep) {
        double tsince = (jdFromUnix(t) - sat.jdEpoch) * 86400.0;
        double pos[3]; sgp4Pos(&sat, tsince, pos);
        double slat, slon, salt;
        eciToLLA(pos, jdFromUnix(t), &slat, &slon, &salt);
        double el = computeElev(obsLat, obsLon, slat, slon, salt);

        if (!inPass) {
            if (havePrev && prevEl < kMinElev && el >= kMinElev) {
                // Refine AOS by linear interpolation in [t-kStep, t]
                double frac = (kMinElev - prevEl) / (el - prevEl);
                aos = (time_t)(t - kStep + frac * kStep);
                aosAz = computeAzim(obsLat, obsLon, slat, slon);
                inPass = true;
                maxEl = el; maxT = t;
            } else if (havePrev && prevEl >= kMinElev && t == startT + kStep) {
                // Already in a pass at scan start.
                aos = startT;
                aosAz = computeAzim(obsLat, obsLon, prevLat, prevLon);
                inPass = true;
                maxEl = prevEl; maxT = startT;
            }
        }

        if (inPass) {
            if (el > maxEl) { maxEl = el; maxT = t; }
            if (el < kMinElev) {
                double frac = (kMinElev - prevEl) / (el - prevEl);
                los = (time_t)(t - kStep + frac * kStep);
                losAz = computeAzim(obsLat, obsLon, slat, slon);
                strncpy(out.name, sat.name, 24); out.name[24] = 0;
                out.norad   = sat.norad;
                out.aos     = aos;
                out.los     = los;
                out.maxElT  = maxT;
                out.maxEl   = (float)maxEl;
                out.aosAz   = (float)aosAz;
                out.losAz   = (float)losAz;
                return true;
            }
        }

        prevEl = el;
        prevLat = slat; prevLon = slon;
        havePrev = true;
    }
    return false;
}

static void computeAllPasses() {
    s_passes.clear();
    if (s_sats.empty()) return;

    auto& cfg = Config::get();
    if (cfg.myGrid.length() < 4) {
        s_status = "no QTH";
        s_ready = true;
        return;
    }
    float obsLatF, obsLonF;
    if (!Geo::gridToLatLon(cfg.myGrid, obsLatF, obsLonF)) {
        s_status = "bad QTH";
        s_ready = true;
        return;
    }
    double obsLat = obsLatF, obsLon = obsLonF;

    time_t now = time(nullptr);
    if (now < 1700000000) { s_status = "no time"; return; }
    time_t end = now + (time_t)kHorizonHours * 3600;

    for (auto& sat : s_sats) {
        time_t t = now;
        // Guard against pathological loops; max ~20 passes per sat per horizon.
        for (int i = 0; i < 25 && t < end; i++) {
            SatPass p;
            if (!computeOnePass(sat, obsLat, obsLon, t, end, p)) break;
            s_passes.push_back(p);
            t = p.los + 60;
        }
    }

    std::sort(s_passes.begin(), s_passes.end(),
              [](const SatPass& a, const SatPass& b){ return a.aos < b.aos; });

    s_status = String((unsigned)s_passes.size()) + " passes";
    s_ready = true;
}

// --- Public lifecycle --------------------------------------------------------

void begin() {
    s_status = "no data";
    if (loadFromFs()) {
        s_status = String((unsigned)s_sats.size()) + " sats (cached)";
    }
}

void loop() {
    uint32_t now = millis();
    static uint32_t lastTry = 0;

    bool need = s_needRefresh
             || s_lastFetchMs == 0
             || (now - s_lastFetchMs > kRefreshMs);

    if (need && WifiMgr::isConnected() && (now - lastTry > kRetryMs)) {
        lastTry = now;
        s_needRefresh = false;
        s_status = "fetching";
        String blob;
        if (fetchTLEs(blob)) {
            String kept = filterAndParse(blob);
            if (!kept.isEmpty()) {
                saveToFs(kept);
                s_lastFetchMs = now;
                s_status = String((unsigned)s_sats.size()) + " sats";
                computeAllPasses();
            } else {
                s_status = "no matches";
            }
        } else {
            s_status = "fetch fail";
        }
    }

    if (!s_sats.empty() && (s_lastComputeMs == 0 || now - s_lastComputeMs > kRecomputeMs)) {
        s_lastComputeMs = now;
        computeAllPasses();
    }
}

}
