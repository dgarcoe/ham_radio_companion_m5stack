#include "geo.h"

#include <math.h>
#include <ctype.h>

namespace Geo {

bool gridToLatLon(const String& g, float& latOut, float& lonOut) {
    if (g.length() < 4) return false;
    char A = (char)toupper(g[0]);
    char B = (char)toupper(g[1]);
    char C = g[2];
    char D = g[3];
    if (A < 'A' || A > 'R') return false;
    if (B < 'A' || B > 'R') return false;
    if (C < '0' || C > '9') return false;
    if (D < '0' || D > '9') return false;

    float lon = (A - 'A') * 20.0f - 180.0f;
    float lat = (B - 'A') * 10.0f -  90.0f;
    lon += (C - '0') * 2.0f;
    lat += (D - '0') * 1.0f;

    if (g.length() >= 6) {
        char E = (char)tolower(g[4]);
        char F = (char)tolower(g[5]);
        if (E < 'a' || E > 'x') return false;
        if (F < 'a' || F > 'x') return false;
        lon += (E - 'a') * (5.0f  / 60.0f);   // 5 minutes per sub-square step
        lat += (F - 'a') * (2.5f  / 60.0f);   // 2.5 minutes per step
        // Centre of the sub-square.
        lon += 2.5f / 60.0f;
        lat += 1.25f / 60.0f;
    } else {
        // Centre of the 2°x1° square.
        lon += 1.0f;
        lat += 0.5f;
    }

    latOut = lat;
    lonOut = lon;
    return true;
}

String latLonToGrid(float lat, float lon) {
    // Normalise.
    if (lat <= -90.0f || lat >= 90.0f)   return "";
    while (lon <= -180.0f) lon += 360.0f;
    while (lon  >  180.0f) lon -= 360.0f;

    float adjLon = lon + 180.0f;     // 0..360
    float adjLat = lat +  90.0f;     // 0..180

    int A = (int)(adjLon / 20.0f);
    int B = (int)(adjLat / 10.0f);
    float remLon = adjLon - A * 20.0f;
    float remLat = adjLat - B * 10.0f;

    int C = (int)(remLon / 2.0f);
    int D = (int)(remLat / 1.0f);
    remLon -= C * 2.0f;
    remLat -= D * 1.0f;

    int E = (int)(remLon / (5.0f / 60.0f));
    int F = (int)(remLat / (2.5f / 60.0f));

    char buf[7];
    buf[0] = 'A' + A;
    buf[1] = 'A' + B;
    buf[2] = '0' + C;
    buf[3] = '0' + D;
    buf[4] = 'a' + E;
    buf[5] = 'a' + F;
    buf[6] = 0;
    return String(buf);
}

static float deg2rad(float d) { return d * (float)M_PI / 180.0f; }
static float rad2deg(float r) { return r * 180.0f / (float)M_PI; }

float distanceKm(float lat1, float lon1, float lat2, float lon2) {
    const float R = 6371.0f; // Earth mean radius, km
    float p1 = deg2rad(lat1);
    float p2 = deg2rad(lat2);
    float dp = deg2rad(lat2 - lat1);
    float dl = deg2rad(lon2 - lon1);
    float a = sinf(dp / 2) * sinf(dp / 2)
            + cosf(p1) * cosf(p2) * sinf(dl / 2) * sinf(dl / 2);
    float c = 2.0f * atan2f(sqrtf(a), sqrtf(1 - a));
    return R * c;
}

float bearingDeg(float lat1, float lon1, float lat2, float lon2) {
    float p1 = deg2rad(lat1);
    float p2 = deg2rad(lat2);
    float dl = deg2rad(lon2 - lon1);
    float y = sinf(dl) * cosf(p2);
    float x = cosf(p1) * sinf(p2)
            - sinf(p1) * cosf(p2) * cosf(dl);
    float br = rad2deg(atan2f(y, x));
    if (br < 0) br += 360.0f;
    return br;
}

const char* compassLabel(float deg) {
    static const char* kPoints[] = {
        "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
    };
    int i = (int)((deg + 11.25f) / 22.5f) & 0x0F;
    return kPoints[i];
}

}
