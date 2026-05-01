#pragma once

#include <Arduino.h>
#include <time.h>

// Sun-position helpers used by the grayline screen.
//
// All math is approximate (good to ~0.5° for the declination and a few
// minutes for the equation of time), which is fine for plotting the
// terminator on a 320x240 display.

namespace Sun {

// Solar declination in degrees for `t` (UTC). Positive = sun north of equator.
float declinationDeg(time_t t);

// Sub-solar longitude in degrees (positive East). At UTC noon on the
// Greenwich meridian this is ~0, drifting -15°/hour.
float subSolarLonDeg(time_t t);

// Whether the sun is currently above the horizon at (lat, lon) decimal
// degrees, given UTC time t. Returns the sun altitude in degrees (positive =
// above horizon).
float altitudeDeg(time_t t, float latDeg, float lonDeg);

// True if the time has been NTP-synced.
bool ready();

}
