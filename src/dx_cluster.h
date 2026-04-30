#pragma once

#include <Arduino.h>
#include <deque>
#include <functional>

struct DxSpot {
    String spotter;        // station that posted the spot
    String dx;             // spotted callsign
    float  freqKHz = 0;    // frequency in kHz
    String mode;           // CW/SSB/FT8/RTTY/... (best-effort)
    String band;           // 80m/40m/.../6m
    String comment;        // free-form remainder
    String timeUtc;        // HHMMZ
    uint32_t rxMillis = 0; // when we received it (for ageing)
};

namespace DxCluster {

void begin();
void loop();

bool isConnected();
String status();              // human-readable connection state

const std::deque<DxSpot>& spots();
size_t spotCount();

// Force a reconnect (e.g. after settings change).
void reconnect();

// Register a callback fired for each newly received spot.
using SpotCallback = std::function<void(const DxSpot&)>;
void onSpot(SpotCallback cb);

// Helper used by the alerts module too.
String bandForFreq(float freqKHz);
String guessMode(float freqKHz);

}
