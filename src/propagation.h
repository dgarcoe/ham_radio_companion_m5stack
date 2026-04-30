#pragma once

#include <Arduino.h>
#include <map>
#include <vector>

struct BandCondition {
    String band;       // "80m-40m", "30m-20m", "17m-15m", "12m-10m"
    String dayCond;    // Poor / Fair / Good
    String nightCond;
};

struct PropagationData {
    bool valid = false;
    String updated;
    String solarFlux;       // SFI
    String aIndex;
    String kIndex;
    String sunspots;
    String xrayClass;
    String solarWind;       // km/s
    String protonFlux;
    String electronFlux;
    String aurora;
    String muf;             // MUF
    String hf;              // overall HF
    String signalNoise;     // S/N noise level

    std::vector<BandCondition> bands;
};

namespace Propagation {

void begin();
void loop();

bool fetchNow();                  // forces a refresh, returns true on success
const PropagationData& data();
uint32_t lastFetchAgeSeconds();   // since last successful fetch
String status();                  // last status text

}
