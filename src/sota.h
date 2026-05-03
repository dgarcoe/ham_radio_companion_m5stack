#pragma once

#include <Arduino.h>
#include <vector>

// Summits On The Air (SOTA) spot fetcher.
//
// Periodically pulls https://api2.sota.org.uk/api/spots/60/all and exposes
// the parsed list. Mirrors the structure of the POTA module.

struct SotaSpot {
    String activator;
    String summit;       // "G/LD-001"
    String summitName;   // "Scafell Pike, 978m"
    float  freqKHz = 0;
    String mode;
    String comments;
    String timeUtc;      // raw ISO-8601 string from the API
    uint32_t rxMillis = 0;
};

namespace Sota {

void begin();
void loop();

bool fetchNow();
const std::vector<SotaSpot>& spots();
String status();
uint32_t lastFetchAgeSeconds();
size_t spotCount();

}
