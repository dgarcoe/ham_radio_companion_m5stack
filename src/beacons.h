#pragma once

#include <Arduino.h>

// NCDXF / IARU International Beacon Project schedule.
//
// 18 stations transmit in turn on each of 5 HF bands. Each station's slot is
// 10 seconds; the full cycle is 18 * 10 = 180 seconds = 3 minutes. Bands are
// staggered by one slot each, so at any moment 5 different stations are live
// (one per band).
//
// All times are derived from time(nullptr) UTC, so this works without any
// network beyond the initial NTP sync that wifi_manager.cpp performs.

namespace Beacons {

struct Station {
    const char* call;     // canonical callsign
    const char* country;  // human-readable QTH
    const char* grid;     // Maidenhead grid square
    float lat;            // decimal degrees, +N
    float lon;            // decimal degrees, +E
};

constexpr int kStationCount = 18;
constexpr int kBandCount    = 5;

// Stations in transmit-order index.
extern const Station kStations[kStationCount];

// Bands in transmit-order index. Frequency in MHz of each beacon's carrier.
extern const float       kBandMHz[kBandCount];
extern const char* const kBandLabel[kBandCount];   // "14", "18", "21", "24", "28"

// Returns the station currently transmitting on band `band` (0..kBandCount-1),
// or -1 if the system clock isn't synced yet (time(nullptr) < 1.7e9).
int currentStationIndex(int band);

// Seconds elapsed (0..9) inside the current 10-second slot, or -1 if time
// isn't synced.
int slotElapsedSeconds();
int slotRemainingSeconds();

// Return the band index a given station is on right now, or -1 if no clock.
int currentBandForStation(int stationIdx);

}
