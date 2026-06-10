// ============================================================
//  sensors.cpp — Dismissal sensor reading
// ============================================================

#include "sensors.h"

void readSensors() {
    // ── Capacitive sensor (T5 / GPIO CAP_SENSOR_PIN) ──────────
    // touchRead returns lower values when something is present (more capacitance).
    // capDetected = true when below CAP_THRESHOLD.
    capRawValue = touchRead(CAP_SENSOR_PIN);
    // Only true when a water bottle is present — human touch (45000+) is intentionally ignored
    capDetected = (capRawValue >= CAP_BOTTLE_MIN && capRawValue <= CAP_BOTTLE_MAX);

    // ── Photoresistor module (GPIO PHOTO_PIN) ─────────────────
    // Digital-output LDR module (DO pin), INPUT_PULLDOWN on ESP32-S2.
    // HIGH = light above module's pot threshold.
    lightDetected = (digitalRead(PHOTO_PIN) == HIGH);

    Serial.printf("[SENSORS] Cap=%lu  Light=%s\n",
                  capRawValue,
                  lightDetected ? "ON" : "off");
}
