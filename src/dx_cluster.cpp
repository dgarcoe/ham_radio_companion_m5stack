#include "dx_cluster.h"

#include <WiFi.h>
#include <vector>
#include <ctype.h>
#include "config.h"
#include "wifi_manager.h"

namespace DxCluster {

static WiFiClient s_client;
static String s_rxBuf;
static uint32_t s_lastAttempt = 0;
static uint32_t s_lastRx = 0;
static bool s_loggedIn = false;
static String s_status = "idle";
static std::deque<DxSpot> s_spots;
static const size_t kMaxSpots = 200;
static std::vector<SpotCallback> s_callbacks;

void onSpot(SpotCallback cb) { s_callbacks.push_back(std::move(cb)); }

bool isConnected() { return s_client.connected() && s_loggedIn; }
String status()    { return s_status; }
const std::deque<DxSpot>& spots() { return s_spots; }
size_t spotCount() { return s_spots.size(); }

String bandForFreq(float kHz) {
    if (kHz >= 1800 && kHz <= 2000)   return "160m";
    if (kHz >= 3500 && kHz <= 4000)   return "80m";
    if (kHz >= 5300 && kHz <= 5410)   return "60m";
    if (kHz >= 7000 && kHz <= 7300)   return "40m";
    if (kHz >= 10100 && kHz <= 10150) return "30m";
    if (kHz >= 14000 && kHz <= 14350) return "20m";
    if (kHz >= 18068 && kHz <= 18168) return "17m";
    if (kHz >= 21000 && kHz <= 21450) return "15m";
    if (kHz >= 24890 && kHz <= 24990) return "12m";
    if (kHz >= 28000 && kHz <= 29700) return "10m";
    if (kHz >= 50000 && kHz <= 54000) return "6m";
    if (kHz >= 144000 && kHz <= 148000) return "2m";
    return "?";
}

// Best-effort mode guess from common digital sub-band conventions.
String guessMode(float kHz) {
    auto inRange = [&](float lo, float hi) { return kHz >= lo && kHz <= hi; };
    if (inRange(7074, 7074.5) || inRange(14074, 14074.5) || inRange(21074, 21074.5) ||
        inRange(28074, 28074.5) || inRange(10136, 10136.5) || inRange(3573, 3573.5) ||
        inRange(18100, 18100.5) || inRange(24915, 24915.5) || inRange(50313, 50313.5))
        return "FT8";
    if (inRange(7056, 7056.5) || inRange(14080, 14080.5)) return "RTTY";

    String b = bandForFreq(kHz);
    // CW segments
    if (b == "160m" && kHz <= 1840) return "CW";
    if (b == "80m"  && kHz <= 3600) return "CW";
    if (b == "40m"  && kHz <= 7040) return "CW";
    if (b == "30m") return "CW";
    if (b == "20m"  && kHz <= 14070) return "CW";
    if (b == "17m"  && kHz <= 18095) return "CW";
    if (b == "15m"  && kHz <= 21070) return "CW";
    if (b == "12m"  && kHz <= 24915) return "CW";
    if (b == "10m"  && kHz <= 28070) return "CW";
    return "SSB";
}

static void connect() {
    if (!WifiMgr::isConnected()) {
        s_status = "waiting wifi";
        return;
    }
    auto& c = Config::get();
    if (c.clusterHost.isEmpty()) {
        s_status = "no host";
        return;
    }
    s_status = String("connecting ") + c.clusterHost + ":" + c.clusterPort;
    s_client.stop();
    s_loggedIn = false;
    s_rxBuf = "";
    if (s_client.connect(c.clusterHost.c_str(), c.clusterPort, 5000)) {
        s_status = "connected, awaiting login prompt";
        s_lastRx = millis();
    } else {
        s_status = "connect failed";
    }
}

static DxSpot parseSpotLine(const String& line) {
    // "DX de SPOTTER:    14025.0  W2XYZ        CW                 ...   1234Z"
    DxSpot sp;
    sp.rxMillis = millis();

    int afterDe = line.indexOf("DX de");
    if (afterDe < 0) return sp;
    int colon = line.indexOf(':', afterDe);
    if (colon < 0) return sp;

    String spotter = line.substring(afterDe + 5, colon);
    spotter.trim();
    sp.spotter = spotter;

    String rest = line.substring(colon + 1);
    rest.trim();

    // Tokenize on whitespace; first token = freq, second = dx call, rest = comment + time.
    int p = 0;
    auto nextTok = [&](String& out) -> bool {
        while (p < (int)rest.length() && isspace(rest[p])) p++;
        int start = p;
        while (p < (int)rest.length() && !isspace(rest[p])) p++;
        if (start == p) return false;
        out = rest.substring(start, p);
        return true;
    };

    String freqStr, dxCall;
    if (!nextTok(freqStr)) return sp;
    if (!nextTok(dxCall))  return sp;

    sp.freqKHz = freqStr.toFloat();
    sp.dx = dxCall;
    sp.band = bandForFreq(sp.freqKHz);

    // Remaining = comment ... time. Time is usually the trailing "HHMMZ".
    while (p < (int)rest.length() && isspace(rest[p])) p++;
    String tail = rest.substring(p);
    tail.trim();

    // Pull trailing time token if present.
    int lastSp = tail.lastIndexOf(' ');
    if (lastSp > 0) {
        String maybeTime = tail.substring(lastSp + 1);
        if (maybeTime.endsWith("Z") && maybeTime.length() >= 4) {
            sp.timeUtc = maybeTime;
            tail = tail.substring(0, lastSp);
            tail.trim();
        }
    }
    sp.comment = tail;

    // Mode: try to extract a known token from the comment, else guess from freq.
    String upper = sp.comment;
    upper.toUpperCase();
    static const char* kModes[] = {"FT8","FT4","CW","SSB","USB","LSB","RTTY","PSK","JT65","JT9","SSTV","AM","FM"};
    for (auto m : kModes) {
        if (upper.indexOf(m) >= 0) { sp.mode = m; break; }
    }
    if (sp.mode.isEmpty()) sp.mode = guessMode(sp.freqKHz);
    return sp;
}

static void handleLine(String line) {
    line.trim();
    if (line.isEmpty()) return;

    // Login prompt: cluster usually asks for "login:" or "Please enter your call".
    if (!s_loggedIn) {
        String l = line;
        l.toLowerCase();
        if (l.indexOf("login") >= 0 || l.indexOf("call") >= 0 || l.indexOf("your call") >= 0) {
            auto& c = Config::get();
            String call = c.myCallsign.length() ? c.myCallsign : String("N0CALL");
            s_client.print(call);
            s_client.print("\r\n");
            s_loggedIn = true;
            s_status = String("logged in as ") + call;
            return;
        }
    }

    if (line.startsWith("DX de") || line.indexOf("DX de") == 0) {
        DxSpot sp = parseSpotLine(line);
        if (sp.dx.length() && sp.freqKHz > 0) {
            s_spots.push_front(sp);
            while (s_spots.size() > kMaxSpots) s_spots.pop_back();
            for (auto& cb : s_callbacks) cb(sp);
        }
    }
}

void begin() {
    s_status = "idle";
}

void loop() {
    if (!WifiMgr::isConnected()) {
        if (s_client.connected()) s_client.stop();
        s_loggedIn = false;
        s_status = "waiting wifi";
        return;
    }

    if (!s_client.connected()) {
        uint32_t now = millis();
        if (now - s_lastAttempt > 10000) {
            s_lastAttempt = now;
            connect();
        }
        return;
    }

    while (s_client.available()) {
        char ch = (char)s_client.read();
        s_lastRx = millis();
        if (ch == '\r') continue;
        if (ch == '\n') {
            handleLine(s_rxBuf);
            s_rxBuf = "";
        } else {
            s_rxBuf += ch;
            if (s_rxBuf.length() > 512) s_rxBuf = ""; // avoid runaway
        }
    }

    // Idle timeout safety net (e.g. cluster died silently).
    if (millis() - s_lastRx > 5UL * 60UL * 1000UL) {
        s_status = "idle timeout, reconnecting";
        s_client.stop();
        s_loggedIn = false;
    }
}

void reconnect() {
    s_client.stop();
    s_loggedIn = false;
    s_lastAttempt = 0;
    s_status = "reconnect requested";
}

}
