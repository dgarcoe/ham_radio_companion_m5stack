#include "sun.h"

#include <math.h>

namespace Sun {

static float deg2rad(float d) { return d * (float)M_PI / 180.0f; }
static float rad2deg(float r) { return r * 180.0f / (float)M_PI; }

bool ready() { return time(nullptr) > 1700000000; }

// Day-of-year (1..366) in UTC for time `t`.
static int dayOfYear(time_t t) {
    struct tm tmv;
    gmtime_r(&t, &tmv);
    return tmv.tm_yday + 1;
}

float declinationDeg(time_t t) {
    // Cooper's formula: simple, accurate to ~0.5 degree.
    float n = (float)dayOfYear(t);
    return -23.44f * cosf(deg2rad((360.0f / 365.0f) * (n + 10.0f)));
}

float subSolarLonDeg(time_t t) {
    // Sub-solar longitude rotates -15°/h with time. Add a tiny equation-of-
    // time correction so noon-by-clock != noon-by-sun by ~+/-15 minutes
    // throughout the year.
    struct tm tmv;
    gmtime_r(&t, &tmv);
    float secondsOfDay = tmv.tm_hour * 3600.0f + tmv.tm_min * 60.0f + tmv.tm_sec;
    float lon = 180.0f - (secondsOfDay / 240.0f);   // 240 sec per degree

    // Equation of time, in minutes (NOAA approximation).
    float n = (float)dayOfYear(t);
    float B = deg2rad((360.0f / 365.0f) * (n - 81.0f));
    float eotMin = 9.87f * sinf(2.0f * B) - 7.53f * cosf(B) - 1.5f * sinf(B);
    lon -= eotMin / 4.0f;   // 1 minute = 0.25 degree

    while (lon >  180.0f) lon -= 360.0f;
    while (lon < -180.0f) lon += 360.0f;
    return lon;
}

float altitudeDeg(time_t t, float latDeg, float lonDeg) {
    float decl = declinationDeg(t);
    float subLon = subSolarLonDeg(t);
    float dRad = deg2rad(decl);
    float lRad = deg2rad(latDeg);
    float HA   = deg2rad(lonDeg - subLon);
    float sinAlt = sinf(lRad) * sinf(dRad)
                 + cosf(lRad) * cosf(dRad) * cosf(HA);
    return rad2deg(asinf(sinAlt));
}

}
