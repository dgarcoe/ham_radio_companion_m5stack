#pragma once

#include <Arduino.h>
#include <vector>

// NOAA Space Weather Prediction Center alert feed:
//   https://services.swpc.noaa.gov/products/alerts.json
//
// Each alert is a free-form text message describing a geomagnetic storm,
// solar radiation event, radio blackout, or watch/warning. We pull it
// periodically and surface the most-recent few on a dedicated screen.

struct NoaaAlert {
    String issued;        // e.g. "2026-04-30 12:34:00.0"
    String code;          // e.g. "ALTK04", "WARK05" - the SWPC product code
    String summary;       // first non-empty content line, summarised
    String message;       // full message body
};

namespace Noaa {

void begin();
void loop();

bool fetchNow();
const std::vector<NoaaAlert>& alerts();
String status();
uint32_t lastFetchAgeSeconds();
size_t count();

}
