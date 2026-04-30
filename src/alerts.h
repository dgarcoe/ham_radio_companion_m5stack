#pragma once

#include <Arduino.h>
#include <deque>
#include "dx_cluster.h"

struct AlertHit {
    String ruleName;
    DxSpot spot;
    uint32_t whenMs;
};

namespace Alerts {

void begin();
void loop();

bool consumeBanner(AlertHit& out);   // pop the most recent unseen alert (for a banner)
const std::deque<AlertHit>& history();
void clearHistory();

}
