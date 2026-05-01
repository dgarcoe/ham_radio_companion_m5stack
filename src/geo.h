#pragma once

#include <Arduino.h>

// Maidenhead grid square <-> lat/lon, plus great-circle bearing and distance.
// All angles in decimal degrees, lat positive North, lon positive East.

namespace Geo {

// Convert a 4 or 6 character Maidenhead grid to the centre lat/lon. Returns
// false if the string isn't a well-formed grid (caller should validate input).
bool gridToLatLon(const String& grid, float& latOut, float& lonOut);

// Convert lat/lon to a 6-char grid square (lowercase sub-square letters).
String latLonToGrid(float lat, float lon);

// Great-circle distance in kilometres between two points.
float distanceKm(float lat1, float lon1, float lat2, float lon2);

// Initial bearing (azimuth) from point 1 to point 2, in degrees clockwise from
// true north (0..360).
float bearingDeg(float lat1, float lon1, float lat2, float lon2);

// 16-point compass label ("N", "NNE", ...) for a bearing.
const char* compassLabel(float bearingDeg);

}
