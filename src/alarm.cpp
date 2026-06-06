// ============================================================
//  alarm.cpp — Alarm triggering, buzzer pulsing, and dismissal
// ============================================================

#include "alarm.h"

// ── triggerAlarm ─────────────────────────────────────────────
// Sets alarmActive and starts buzzer pulsing. Clears leftover dismiss state
// so a previously interrupted debounce doesn't carry over to the new ring.
void triggerAlarm() {
    alarmActive       = true;     // Arm the alarm — loop will call updateBuzzerPulse each iteration
    buzzerOn          = true;     // Start with buzzer on (first half of the first pulse)
    lastBuzzerToggle  = millis(); // Reference timestamp for the first toggle interval
    dismissInProgress = false;    // Clear any remnant debounce state from a previous alarm
    dismissStartMs    = 0;
    digitalWrite(BUZZER_PIN, HIGH); // HIGH → 2N2222 base current → transistor on → buzzer sounds
    Serial.println("[ALARM] TRIGGERED — buzzer pulsing started");
}

// ── updateBuzzerPulse ─────────────────────────────────────────
// Non-blocking buzzer toggle using millis(). Switches between ON and OFF states
// according to BUZZER_ON_MS and BUZZER_OFF_MS. Must be called every loop iteration
// while alarmActive — the short delay in loop() keeps the timing accurate.
void updateBuzzerPulse() {
    unsigned long now     = millis();
    unsigned long elapsed = now - lastBuzzerToggle; // Time spent in the current on/off state

    if (buzzerOn && elapsed >= BUZZER_ON_MS) {
        // Buzzer has been on for the full ON duration — switch it off
        buzzerOn = false;
        digitalWrite(BUZZER_PIN, LOW);  // LOW → transistor off → buzzer silent
        lastBuzzerToggle = now;         // Reset the interval timer for the OFF phase
        Serial.println("[ALARM] Buzzer OFF");

    } else if (!buzzerOn && elapsed >= BUZZER_OFF_MS) {
        // Buzzer has been off for the full OFF duration — switch it back on
        buzzerOn = true;
        digitalWrite(BUZZER_PIN, HIGH); // HIGH → transistor on → buzzer sounds
        lastBuzzerToggle = now;         // Reset the interval timer for the ON phase
        Serial.println("[ALARM] Buzzer ON");
    }
    // Not yet at the toggle threshold: hold current state, nothing to do
}

// ── handleAlarmDismiss ────────────────────────────────────────
// Implements the 2-second debounce for alarm dismissal.
// All three conditions (light + bottle present + spring extended) must be simultaneously
// true for DISMISS_DEBOUNCE_MS before the alarm is silenced.
// Must be called after readSensors() each loop iteration.
void handleAlarmDismiss() {
    // Check whether all three dismissal conditions are currently satisfied
    bool allMet = lightDetected && bottlePresent && springExtended;

    if (allMet && !dismissInProgress) {
        // Conditions just became fully satisfied for the first time — start the debounce clock
        dismissInProgress = true;
        dismissStartMs    = millis();
        Serial.println("[ALARM] All dismiss conditions met — starting 2 s debounce");

    } else if (allMet && dismissInProgress) {
        // Still satisfied — check whether the debounce window has fully elapsed
        if (millis() - dismissStartMs >= DISMISS_DEBOUNCE_MS) {
            // Held for the full debounce period — dismiss the alarm
            alarmActive       = false;
            dismissInProgress = false;
            buzzerOn          = false;
            digitalWrite(BUZZER_PIN, LOW); // Silence immediately; don't wait for next pulse edge
            Serial.println("[ALARM] DISMISSED — conditions held 2 s. Buzzer silenced.");
        }
        // Debounce timer still running — hold and wait

    } else if (!allMet && dismissInProgress) {
        // One or more conditions dropped before debounce elapsed — reset the countdown
        dismissInProgress = false;
        dismissStartMs    = 0;
        Serial.println("[ALARM] Dismiss conditions broken — debounce reset");
    }
    // !allMet && !dismissInProgress: idle, nothing to do
}
