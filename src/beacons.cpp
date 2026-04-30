#include "beacons.h"

#include <time.h>

namespace Beacons {

const Station kStations[kStationCount] = {
    { "4U1UN",  "United Nations, NY",   "FN30as",  40.749f,  -73.968f },
    { "VE8AT",  "Inuvik, NWT, Canada",  "EQ79ax",  68.376f, -133.501f },
    { "W6WX",   "Mt Umunhum, CA, USA",  "CM97bd",  37.165f, -121.901f },
    { "KH6RS",  "Maui, Hawaii",         "BL10ts",  20.785f, -156.500f },
    { "ZL6B",   "Mt Climie, NZ",        "RE78tw", -41.119f,  175.221f },
    { "VK6RBP", "Rolystone, Australia", "OF87av", -32.092f,  116.046f },
    { "JA2IGY", "Mt Asama, Japan",      "PM84jk",  34.448f,  136.794f },
    { "RR9O",   "Novosibirsk, Russia",  "NO14kx",  54.987f,   82.898f },
    { "VR2B",   "Hong Kong",            "OL72bg",  22.262f,  114.255f },
    { "4S7B",   "Colombo, Sri Lanka",   "MJ96wv",   6.906f,   79.866f },
    { "ZS6DN",  "Pretoria, S. Africa",  "KG44dc", -25.913f,   28.260f },
    { "5Z4B",   "Kenya",                "KI88ks",  -1.295f,   36.812f },
    { "4X6TU",  "Tel Aviv, Israel",     "KM72jb",  32.108f,   34.806f },
    { "OH2B",   "Lohja, Finland",       "KP20le",  60.246f,   24.066f },
    { "CS3B",   "Madeira",              "IM12or",  32.700f,  -16.785f },
    { "LU4AA",  "Buenos Aires, Argentina","GF05tj",-34.611f,  -58.371f },
    { "OA4B",   "Lima, Peru",           "FH17mw", -12.046f,  -77.030f },
    { "YV5B",   "Caracas, Venezuela",   "FK60nl",  10.490f,  -66.852f },
};

const float kBandMHz[kBandCount] = {
    14.100f, 18.110f, 21.150f, 24.930f, 28.200f
};
const char* const kBandLabel[kBandCount] = {
    "14", "18", "21", "24", "28"
};

static bool clockSynced() {
    return time(nullptr) > 1700000000;
}

static int cycleSecond() {
    // Seconds since the most recent 3-minute boundary.
    time_t now = time(nullptr);
    return (int)(now % 180);
}

int currentStationIndex(int band) {
    if (!clockSynced()) return -1;
    if (band < 0 || band >= kBandCount) return -1;
    int slot = cycleSecond() / 10;            // 0..17
    int idx  = (slot - band + kStationCount) % kStationCount;
    return idx;
}

int slotElapsedSeconds() {
    if (!clockSynced()) return -1;
    return cycleSecond() % 10;
}

int slotRemainingSeconds() {
    int e = slotElapsedSeconds();
    return e < 0 ? -1 : (10 - e);
}

int currentBandForStation(int stationIdx) {
    if (!clockSynced()) return -1;
    if (stationIdx < 0 || stationIdx >= kStationCount) return -1;
    int slot = cycleSecond() / 10;
    // station = (slot - band + 18) % 18  =>  band = (slot - station) mod 18
    // valid only if that diff falls in [0, kBandCount).
    int diff = (slot - stationIdx + kStationCount) % kStationCount;
    if (diff < kBandCount) return diff;
    return -1;
}

}
