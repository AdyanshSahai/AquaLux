#pragma once
// ============================================================
//  sensors.h — Alarm-dismissal sensor reading
// ============================================================

#include "globals.h"

// Reads all three dismissal inputs (light, IR, spring) and
// updates bottlePresent, springExtended, lightDetected globals.
// Call every loop iteration while alarmActive, and once at boot.
void readSensors();
