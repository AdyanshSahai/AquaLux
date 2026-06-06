/*
 * ============================================================
 *  AquaLux — Hydration Alarm System
 *  Platform : ESP32-S2 Mini (LOLIN / Wemos S2 Mini)
 *  Framework: Arduino (PlatformIO)
 * ============================================================
 *
 *  Boot flow
 *  ─────────
 *  1. Init pins + Serial
 *  2. Load Preferences (WiFi creds + alarm time)
 *  3. If creds found  → connect home WiFi → NTP sync → disconnect → AP mode
 *     If no creds     → AP mode immediately (first-time setup)
 *  4. Serve web UI at 192.168.4.1: set alarm, update WiFi, view sensor states
 *  5. At alarm time   → pulse buzzer via 2N2222 (500 ms on / 500 ms off)
 *  6. Alarm dismiss   → light ON + bottle present + spring extended, all held >= 2 s
 *  7. 3 am daily      → brief home-WiFi reconnect for NTP resync
 *  8. Reset button    → hold 3 s → wipe Preferences → beep → reboot
 *
 *  File layout
 *  ───────────
 *  config.h        — all pin and timing constants (#defines)
 *  globals.h/.cpp  — shared state variables (extern declarations + definitions)
 *  wifi_ntp.h/.cpp — parseAlarmTime, loadPreferences, syncNTP
 *  sensors.h/.cpp  — readSensors
 *  alarm.h/.cpp    — triggerAlarm, updateBuzzerPulse, handleAlarmDismiss
 *  webserver.h/.cpp— embedded HTML + HTTP route handlers
 *  reset_button.h/.cpp — handleResetButton
 *  main.cpp        — setup() and loop() (this file)
 *
 *  NOTE — light sleep: cannot be used while the AsyncWebServer AP is active.
 *  Light sleep suspends the WiFi radio, which disconnects AP clients.
 *  Using delay() instead — it yields to FreeRTOS background tasks (WiFi stack,
 *  web server handler task, watchdog) without touching the radio.
 */

// ── Pull in all subsystems ────────────────────────────────────
// Each header transitively includes globals.h → all library headers + config.h,
// so this list is all that is needed here.
#include "wifi_ntp.h"
#include "sensors.h"
#include "alarm.h"
#include "webserver.h"
#include "reset_button.h"

// ============================================================
//  setup()
// ============================================================

void setup() {
    // ── Serial monitor ────────────────────────────────────────
    Serial.begin(115200); // 115200 baud — must match monitor_speed in platformio.ini
    delay(500);           // Wait for serial monitor to attach before printing boot messages
    Serial.println("\n===== AquaLux BOOT =====");

    // ── Pin configuration ─────────────────────────────────────
    pinMode(BUZZER_PIN,         OUTPUT);
    digitalWrite(BUZZER_PIN,    LOW);          // Explicitly off; prevents startup beep if pin floats

    pinMode(IR_PRESENCE_PIN,    INPUT);        // IR module has on-board pull-up — do NOT add internal
    pinMode(SPRING_SWITCH_PIN,  INPUT_PULLUP); // Internal pull-up; spring HIGH when free (bottle empty)
    pinMode(LIGHT_BUTTON_PIN,   INPUT_PULLUP); // Internal pull-up; LOW when button pressed (temp sub)
    pinMode(RESET_PIN,          INPUT_PULLUP); // Internal pull-up; LOW = button pressed = counting down

    Serial.println("[SETUP] Pins configured");

    // ── Load stored configuration from NVS ────────────────────
    bool hasCreds = loadPreferences(); // Populates storedSSID, storedPassword, alarmHour, alarmMinute

    // ── Initialise NTP UDP socket ─────────────────────────────
    // begin() just opens the UDP port — no WiFi needed yet; actual sync is inside syncNTP()
    timeClient.begin();

    // ── NTP sync or first-time AP setup ───────────────────────
    if (hasCreds) {
        // Credentials available — sync time before opening the AP so the clock is correct immediately
        Serial.println("[SETUP] Credentials found — running NTP sync");
        syncNTP(); // Connects → syncs → disconnects → sets WIFI_AP → calls WiFi.softAP()
    } else {
        // No credentials stored — go straight to AP mode for first-time configuration
        Serial.println("[SETUP] No credentials — entering AP setup mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID); // Open AP, no password
        Serial.printf("[SETUP] AP up: SSID='%s'  IP=%s\n", AP_SSID, AP_GATEWAY_IP);
    }

    // ── Start web server ──────────────────────────────────────
    // AP must be running before this; server.begin() registers with the WiFi event loop
    setupWebServer();

    // ── Initial sensor snapshot ───────────────────────────────
    // Populate the global sensor booleans so the /status endpoint has real values immediately
    readSensors();

    Serial.printf("[SETUP] Boot complete | Alarm=%02d:%02d | Time known=%s\n",
                  alarmHour, alarmMinute,
                  timeClient.isTimeSet() ? "YES" : "NO (no creds yet)");
    Serial.println("===== AquaLux RUNNING =====\n");
}

// ============================================================
//  loop()
// ============================================================

void loop() {
    // ── 1. Reset button ───────────────────────────────────────
    // Checked every iteration — highest priority so a reset can interrupt even an active alarm
    handleResetButton();

    // ── 2. Time-dependent logic (only valid once NTP has synced) ──
    if (timeClient.isTimeSet()) {
        int curHour   = timeClient.getHours();   // 0–23, respects NTP_UTC_OFFSET_SEC
        int curMinute = timeClient.getMinutes(); // 0–59

        // ── 2a. Daily 3 am NTP resync ─────────────────────────
        // Fires once per day at exactly NTP_RESYNC_HOUR:NTP_RESYNC_MINUTE
        if (curHour == NTP_RESYNC_HOUR && curMinute == NTP_RESYNC_MINUTE && !ntpResynced3am) {
            Serial.println("[LOOP] 3 am NTP resync triggered");
            syncNTP();             // Reconnect home WiFi, resync, disconnect, restore AP
            ntpResynced3am = true; // Block re-trigger for the rest of this minute
        } else if (!(curHour == NTP_RESYNC_HOUR && curMinute == NTP_RESYNC_MINUTE)) {
            // Reset flag once we leave the 3 am minute so it fires again tomorrow
            ntpResynced3am = false;
        }

        // ── 2b. Alarm trigger ─────────────────────────────────
        if (curHour == alarmHour && curMinute == alarmMinute) {
            if (!alarmActive && !alarmFiredThisMinute) {
                // Fire once per clock-minute; alarmFiredThisMinute prevents re-trigger if the
                // alarm is dismissed and the minute hasn't changed yet (e.g. dismissed at 07:00:30)
                Serial.printf("[LOOP] Alarm time %02d:%02d reached — triggering\n", curHour, curMinute);
                triggerAlarm();
                alarmFiredThisMinute = true;
            }
        } else {
            // Outside the alarm minute — clear the one-shot guard so it fires again tomorrow
            alarmFiredThisMinute = false;
        }
    }

    // ── 3. Active alarm handling ──────────────────────────────
    if (alarmActive) {
        readSensors();        // Refresh bottlePresent, springExtended, lightDetected from hardware
        updateBuzzerPulse();  // Toggle buzzer ON/OFF per BUZZER_ON_MS / BUZZER_OFF_MS (non-blocking)
        handleAlarmDismiss(); // Run 2 s debounce; silences alarm if all conditions held long enough
    }

    // ── 4. Yield delay ────────────────────────────────────────
    // Light sleep is NOT used: it suspends the WiFi radio, which disconnects AP clients and
    // pauses the AsyncWebServer. delay() yields the CPU to all FreeRTOS background tasks
    // (WiFi stack, web server handler task, watchdog timer) without disturbing the radio.
    delay(alarmActive ? LOOP_ALARM_DELAY_MS : LOOP_IDLE_DELAY_MS);
}
