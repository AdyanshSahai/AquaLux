// ============================================================
//  alarm.cpp — Alarm triggering, buzzer pulsing, and dismissal
// ============================================================

#include "alarm.h"

void triggerAlarm() {
    alarmActive       = true;
    buzzerOn          = true;
    lastBuzzerToggle  = millis();
    dismissInProgress = false;
    dismissStartMs    = 0;
    digitalWrite(BUZZER_PIN, HIGH); // HIGH → transistor on → buzzer sounds
    Serial.println("[ALARM] TRIGGERED — buzzer pulsing started");
}

void updateBuzzerPulse() {
    unsigned long now     = millis();
    unsigned long elapsed = now - lastBuzzerToggle;

    if (buzzerOn && elapsed >= BUZZER_ON_MS) {
        buzzerOn = false;
        digitalWrite(BUZZER_PIN, LOW);  // LOW → transistor off → silent
        lastBuzzerToggle = now;
        Serial.println("[ALARM] Buzzer OFF");

    } else if (!buzzerOn && elapsed >= BUZZER_OFF_MS) {
        buzzerOn = true;
        digitalWrite(BUZZER_PIN, HIGH); // HIGH → transistor on → buzzer sounds
        lastBuzzerToggle = now;
        Serial.println("[ALARM] Buzzer ON");
    }
}

void handleAlarmDismiss() {
    bool allMet = lightDetected && capDetected;

    if (allMet && !dismissInProgress) {
        dismissInProgress = true;
        dismissStartMs    = millis();
        Serial.println("[ALARM] All dismiss conditions met — starting 2 s debounce");

    } else if (allMet && dismissInProgress) {
        if (millis() - dismissStartMs >= DISMISS_DEBOUNCE_MS) {
            alarmActive       = false;
            dismissInProgress = false;
            buzzerOn          = false;
            digitalWrite(BUZZER_PIN, LOW); // Silence immediately
            Serial.println("[ALARM] DISMISSED — conditions held 2 s. Buzzer silenced.");
        }

    } else if (!allMet && dismissInProgress) {
        dismissInProgress = false;
        dismissStartMs    = 0;
        Serial.println("[ALARM] Dismiss conditions broken — debounce reset");
    }
}
