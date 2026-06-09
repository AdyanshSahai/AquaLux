// ============================================================
//  reset_button.cpp — Physical factory-reset button handler
// ============================================================

#include "reset_button.h"

// ── handleResetButton ─────────────────────────────────────────
// Non-blocking hold-detection for RESET_PIN.
// Called every loop iteration (highest priority — can interrupt an active alarm).
// Hold >= RESET_HOLD_MS → wipe all NVS keys → confirmation beep → reboot to setup mode.
void handleResetButton() {
    bool pressed = (digitalRead(RESET_PIN) == LOW); // Active LOW: LOW = button physically held down

    if (pressed && !resetHeldTracking) {
        // Button just went down — start the hold timer
        resetHeldTracking = true;
        resetPressedMs    = millis();
        Serial.println("[RESET] Button pressed — hold 3 s for factory reset");

    } else if (!pressed && resetHeldTracking) {
        // Button released before the threshold — user changed their mind
        resetHeldTracking = false;
        resetPressedMs    = 0;
        Serial.println("[RESET] Released early — reset cancelled");

    } else if (pressed && resetHeldTracking) {
        // Button still held — check if it has been held long enough
        if (millis() - resetPressedMs >= RESET_HOLD_MS) {
            Serial.println("[RESET] Threshold reached — wiping Preferences");

            preferences.begin(PREF_NAMESPACE, false); // Open NVS read-write
            preferences.clear();                       // Delete every key in the aqualux namespace
            preferences.end();                         // Flush to flash and release handle

            digitalWrite(BUZZER_PIN, HIGH);  // Beep on
            delay(RESET_BEEP_MS);
            digitalWrite(BUZZER_PIN, LOW);   // Beep off

            Serial.println("[RESET] Rebooting to setup mode");
            delay(100);      // Let the serial buffer flush before the restart
            ESP.restart();   // Reboot — empty NVS → device enters AP setup mode
        }
        // Hold threshold not yet reached — keep tracking next iteration
    }
}
