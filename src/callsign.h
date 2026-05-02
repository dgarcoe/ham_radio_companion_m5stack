#pragma once

#include <Arduino.h>

namespace Callsign {

// Best-effort continent lookup from a ham callsign. Returns one of
// "NA", "SA", "EU", "AF", "AS", "OC", "AN" or nullptr if unknown.
// Strips any "/portable" or "/MM" designator before matching.
const char* continent(const String& call);

}
