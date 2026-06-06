// ============================================================
//  sensors.cpp — Alarm-dismissal sensor reading
// ============================================================

#include "sensors.h"

// ── readSensors ───────────────────────────────────────────────
// Reads all three alarm-dismissal inputs and updates the global sensor booleans.
// Called every loop iteration while alarmActive, and once at boot for UI correctness.
void readSensors() {

    // ── Light / wake detection ────────────────────────────────
    // Photoresistor on analog pin PHOTO_PIN in a voltage-divider with 10 kΩ to GND.
    // Bright light → low resistance → high voltage → high ADC reading.
    // Above LIGHT_THRESHOLD = lights are on (alarm dismissal condition satisfied).
    int  photoRaw = analogRead(PHOTO_PIN);           // 12-bit ADC: 0 = dark, 4095 = max brightness
    photoRawValue = photoRaw;                        // Store for /status endpoint calibration display
    lightDetected = (photoRaw > LIGHT_THRESHOLD);    // Above threshold = ambient light present
    Serial.printf("[SENSOR] Photo ADC=%d  threshold=%d\n", photoRaw, LIGHT_THRESHOLD);

    // ── COMMENTED OUT: temporary pushbutton substitution ──────
    // TEMPORARY SUBSTITUTION — uncomment below and comment out the analogRead block above
    // if the photoresistor is unavailable and you need to fall back to a pushbutton on LIGHT_BUTTON_PIN
    // #define LIGHT_BUTTON_PIN  XX   // digital input, internal pull-up, active LOW
    // bool lightRaw = digitalRead(LIGHT_BUTTON_PIN);
    // lightDetected = (lightRaw == LOW);             // Active LOW: LOW = button pressed = lights on

    // ── IR presence detection ─────────────────────────────────
    bool irRaw    = digitalRead(IR_PRESENCE_PIN); // Module output: HIGH when object reflects IR
    bottlePresent = (irRaw == HIGH);               // HIGH = bottle blocking / reflecting IR beam

    // ── Weight / spring-switch state ──────────────────────────
    // CUSTOM SPRING SWITCH — one side wired to 3.3V, other side to SPRING_SWITCH_PIN (INPUT_PULLDOWN)
    // Open (bottle on platform, spring compressed) → pin pulled to GND by internal pulldown → LOW
    // Closed (bottle removed, spring extends and makes contact with 3.3V rail) → HIGH
    bool springRaw = digitalRead(SPRING_SWITCH_PIN);
    springExtended = (springRaw == HIGH); // HIGH = switch closed = spring extended = bottle emptied/removed

    Serial.printf("[SENSORS] Light=%-3s  Bottle=%-7s  Spring=%s\n",
                  lightDetected  ? "ON"      : "off",
                  bottlePresent  ? "PRESENT" : "absent",
                  springExtended ? "EMPTY"   : "heavy");
}
