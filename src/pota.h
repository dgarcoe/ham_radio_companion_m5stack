#pragma once

#include <Arduino.h>
#include <vector>

// Simple Parks On The Air (POTA) spot fetcher.
//
// Periodically pulls https://api.pota.app/spot/activator (a JSON array of the
// currently-spotted activators) and exposes the parsed list. Designed to feed
// both a dedicated screen and the home launcher's status badge.

struct PotaSpot {
    String activator;
    String reference;     // "K-1234"
    String parkName;
    String location;      // location-desc, e.g. "US-CA"
    float  freqKHz = 0;
    String mode;
    String spotter;
    String comments;
    String spotTime;      // raw ISO-8601 string from the API
    uint32_t rxMillis = 0;
};

namespace Pota {

void begin();
void loop();

bool fetchNow();                    // returns true on a successful refresh
const std::vector<PotaSpot>& spots();
String status();                    // human-readable last-fetch status
uint32_t lastFetchAgeSeconds();     // UINT32_MAX before first success
size_t spotCount();

}
