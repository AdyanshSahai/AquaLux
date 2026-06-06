#pragma once
// ============================================================
//  alarm.h — Alarm triggering, buzzer pulsing, and dismissal
// ============================================================

#include "globals.h"

// Activates the alarm: sets alarmActive, drives BUZZER_PIN HIGH,
// resets dismiss debounce state.
void triggerAlarm();

// Non-blocking buzzer on/off toggle based on BUZZER_ON_MS / BUZZER_OFF_MS.
// Must be called every loop iteration while alarmActive.
void updateBuzzerPulse();

// Checks whether all three dismissal conditions have been held
// simultaneously for DISMISS_DEBOUNCE_MS and silences the alarm if so.
// Must be called after readSensors() each loop iteration.
void handleAlarmDismiss();
