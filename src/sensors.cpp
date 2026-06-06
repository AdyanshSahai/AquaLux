// ============================================================
//  sensors.cpp — Alarm-dismissal sensor reading
// ============================================================

#include "sensors.h"

// ── readSensors ───────────────────────────────────────────────
// Reads all three alarm-dismissal inputs and updates the global sensor booleans.
// Called every loop iteration while alarmActive, and once at boot for UI correctness.
void readSensors() {

    // ── Light / wake detection ────────────────────────────────
    // TEMPORARY SUBSTITUTION — pushbutton on LIGHT_BUTTON_PIN stands in for photoresistor.
    // Active LOW (internal pull-up enabled): pin LOW = button pressed = "lights are on".
    bool lightRaw = digitalRead(LIGHT_BUTTON_PIN); // LOW = pressed, HIGH = released
    lightDetected = (lightRaw == LOW);              // Invert: active LOW means LOW = detected

    // ── COMMENTED OUT: photoresistor analog read ──────────────
    // TEMPORARY SUBSTITUTION — uncomment and replace with photoresistor on analog pin when available
    // int  photoRaw = analogRead(PHOTO_PIN);                    // 12-bit ADC: 0 = dark, 4095 = max brightness
    // lightDetected = (photoRaw > LIGHT_THRESHOLD);             // Above threshold = ambient light present
    // Serial.printf("[SENSOR] Photo ADC=%d  threshold=%d\n", photoRaw, LIGHT_THRESHOLD);

    // ── IR presence detection ─────────────────────────────────
    bool irRaw    = digitalRead(IR_PRESENCE_PIN); // Module output: HIGH when object reflects IR
    bottlePresent = (irRaw == HIGH);               // HIGH = bottle blocking / reflecting IR beam

    // ── Weight / spring-switch state ──────────────────────────
    // CUSTOM SPRING SWITCH — reads LOW when bottle weight compresses spring, HIGH when bottle is empty and spring extends
    bool springRaw = digitalRead(SPRING_SWITCH_PIN); // Pulled up internally; goes LOW when compressed
    springExtended = (springRaw == HIGH);             // HIGH = spring free = bottle has been emptied / lifted

    Serial.printf("[SENSORS] Light=%-3s  Bottle=%-7s  Spring=%s\n",
                  lightDetected  ? "ON"      : "off",
                  bottlePresent  ? "PRESENT" : "absent",
                  springExtended ? "EMPTY"   : "heavy");
}
