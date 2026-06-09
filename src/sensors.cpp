// ============================================================
//  sensors.cpp — Dismissal sensor reading
// ============================================================

#include "sensors.h"

void readSensors() {
    // ── Capacitive sensor (GPIO CAP_SENSOR_PIN) ───────────────
    // External module with digital output: HIGH = presence/touch detected
    capDetected = (digitalRead(CAP_SENSOR_PIN) == HIGH);

    // ── Photoresistor module (GPIO PHOTO_PIN) ─────────────────
    // Digital-output LDR module (DO pin): HIGH = light above module's pot threshold
    lightDetected = (digitalRead(PHOTO_PIN) == HIGH);

    Serial.printf("[SENSORS] Cap=%-3s  Light=%s\n",
                  capDetected   ? "ON"  : "off",
                  lightDetected ? "ON"  : "off");
}
