#pragma once

#include <Arduino.h>
#include <time.h>
#include <vector>

struct SatPass {
    char   name[25];
    int    norad;
    time_t aos;       // acquisition of signal (UTC)
    time_t maxElT;    // time of maximum elevation
    time_t los;       // loss of signal
    float  maxEl;     // max elevation (deg)
    float  aosAz;     // azimuth at AOS (deg)
    float  losAz;     // azimuth at LOS (deg)
};

namespace Satellites {

void   begin();
void   loop();

// True once we've at least computed a pass list (even an empty one).
bool   ready();

// Sorted by AOS, soonest first. Empty if no QTH set or no TLEs loaded.
const std::vector<SatPass>& passes();

// Short status string for the launcher tile.
String status();

// Force a TLE refresh on the next loop.
void   requestRefresh();

uint32_t lastFetchAgeSeconds();
size_t   tleCount();

}
