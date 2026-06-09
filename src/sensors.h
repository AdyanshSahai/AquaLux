#pragma once
// ============================================================
//  sensors.h — Alarm-dismissal sensor reading
// ============================================================

#include "globals.h"

// Reads the capacitive sensor (GPIO CAP_SENSOR_PIN) and digital photoresistor
// (GPIO PHOTO_PIN) and updates capDetected, lightDetected globals.
// Call every loop iteration while alarmActive, and once at boot.
void readSensors();
