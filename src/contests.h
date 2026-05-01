#pragma once

#include <Arduino.h>
#include <vector>
#include <time.h>

// Major HF contests calendar.
//
// The schedule is fully offline: each contest has a recurrence rule (e.g.
// "first Saturday of November, 48 hours"), and we compute the next occurrence
// from the system clock. Once NTP has synced (a few seconds after WiFi),
// dates are accurate. The list is recomputed automatically when the UTC date
// rolls over.

struct Contest {
    String name;     // e.g. "CQ WW DX SSB"
    String mode;     // "CW", "SSB", "RTTY", "MIX"
    time_t startUtc; // start of contest window
    time_t endUtc;   // end of contest window
};

namespace Contests {

void begin();
void loop();

// Upcoming contests, sorted by startUtc ascending. Includes contests that are
// currently "running" (started but not yet ended).
const std::vector<Contest>& upcoming();

// True if the system clock has been synced and the list is meaningful.
bool ready();

// Forces a recompute (useful after manual time changes).
void refresh();

}
