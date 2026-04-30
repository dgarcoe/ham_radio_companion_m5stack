#include "alerts.h"

#include <M5Unified.h>
#include "config.h"
#include "dx_cluster.h"

namespace Alerts {

static std::deque<AlertHit> s_hits;
static std::deque<AlertHit> s_pendingBanners;
static const size_t kMaxHistory = 50;

static bool ruleMatches(const AlertRule& r, const DxSpot& s) {
    if (!r.enabled) return false;
    if (r.band.length() && r.band != "any" && r.band != s.band) return false;

    if (r.mode.length() && r.mode != "any") {
        String want = r.mode; want.toUpperCase();
        String have = s.mode; have.toUpperCase();
        if (want != have) return false;
    }

    if (r.prefix.length()) {
        String pfx = r.prefix; pfx.toUpperCase();
        String dx  = s.dx;     dx.toUpperCase();
        if (!dx.startsWith(pfx)) return false;
    }

    if (r.callMatch.length()) {
        String needle = r.callMatch; needle.toUpperCase();
        String hay    = s.dx;        hay.toUpperCase();
        if (hay.indexOf(needle) < 0) return false;
    }
    return true;
}

static void onSpot(const DxSpot& sp) {
    auto& cfg = Config::get();
    for (auto& r : cfg.alerts) {
        if (ruleMatches(r, sp)) {
            AlertHit h;
            h.ruleName = r.name;
            h.spot = sp;
            h.whenMs = millis();
            s_hits.push_front(h);
            while (s_hits.size() > kMaxHistory) s_hits.pop_back();
            s_pendingBanners.push_back(h);

            if (cfg.soundEnabled) {
                M5.Speaker.tone(2000, 120);
                delay(140);
                M5.Speaker.tone(2600, 120);
            }
            break; // one alert per spot is enough
        }
    }
}

void begin() {
    DxCluster::onSpot(onSpot);
}

void loop() { /* nothing periodic for now */ }

bool consumeBanner(AlertHit& out) {
    if (s_pendingBanners.empty()) return false;
    out = s_pendingBanners.front();
    s_pendingBanners.pop_front();
    return true;
}

const std::deque<AlertHit>& history() { return s_hits; }
void clearHistory() { s_hits.clear(); s_pendingBanners.clear(); }

}
