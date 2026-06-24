#pragma once
// ============================================================
//  globals.h — extern declarations for all shared state
//
//  This header is the single place that pulls in all library
//  includes. Every other header just includes this one and
//  gets everything transitively — no repeated include lists.
//
//  Actual variable *definitions* live in globals.cpp.
//  Including this header declares the variables without
//  defining them (no duplicate-symbol linker errors).
// ============================================================

#include <Arduino.h>           // String, bool, uint types, millis, Serial, delay
#include <WiFi.h>              // WiFi AP/STA control
#include <Preferences.h>       // ESP32 NVS key-value store
#include <ESPAsyncWebServer.h> // Async HTTP server type
#include <AsyncTCP.h>          // Async TCP layer (required by ESPAsyncWebServer)
#include <NTPClient.h>         // NTP time client
#include <WiFiUdp.h>           // UDP socket used by NTPClient

#include "config.h"            // All pin and timing constants

// ── Library object instances ──────────────────────────────────
extern Preferences    preferences;  // NVS handle
extern AsyncWebServer server;       // HTTP server on port 80
extern WiFiUDP        ntpUDP;       // UDP socket owned by NTPClient
extern NTPClient      timeClient;   // NTP time client; advances via millis() between syncs

// ── Persisted configuration (loaded from NVS at boot) ────────
extern String storedSSID;      // Home WiFi SSID; empty string = no credentials stored
extern String storedPassword;  // Home WiFi password
extern String alarmTimeStr;    // Alarm time as "HH:MM" string (used in web UI and /status JSON)
extern int    alarmHour;       // Parsed integer hour  (0–23)
extern int    alarmMinute;     // Parsed integer minute (0–59)

// ── Alarm runtime state ───────────────────────────────────────
extern bool          alarmActive;           // true while alarm is ringing and not yet dismissed
extern bool          buzzerOn;              // Current physical state of BUZZER_PIN
extern unsigned long lastBuzzerToggle;      // millis() at last buzzer on↔off transition
extern bool          alarmFiredThisMinute;  // One-shot guard — prevents re-trigger if alarm is
                                            // dismissed and the clock is still in the same minute

// ── Dismiss debounce state ────────────────────────────────────
extern bool          dismissInProgress;  // true = 2 s debounce countdown running
extern unsigned long dismissStartMs;     // millis() when all three conditions first became satisfied

// ── Sensor readings (updated each loop iteration) ─────────────
extern uint32_t capRawValue;   // Raw touchRead() value — lower = more capacitance = touched
extern bool     capDetected;   // true when capRawValue is within [capBottleMin, capBottleMax]
extern bool     lightDetected; // Photoresistor module DO HIGH = ambient light present

// ── Capacitive threshold (runtime, loaded from NVS or default) ─
extern uint32_t capBottleMin;  // Lower bound for bottle detection (default CAP_BOTTLE_MIN)
extern uint32_t capBottleMax;  // Upper bound for bottle detection (default CAP_BOTTLE_MAX)

// ── WiFi connection timeout (runtime, configurable via Config panel) ──
extern uint32_t ntpConnectTimeoutMs; // How long to wait for home WiFi before giving up

// ── Manual time tracking ──────────────────────────────────────
extern bool          manualTimeSet; // true once user sets time via Config panel
extern unsigned long manualSetAt;   // millis() when manual time was recorded
extern int           manualHour;    // 0-23
extern int           manualMinute;  // 0-59
extern int           manualSecond;  // 0-59

// ── Daily NTP resync tracking ─────────────────────────────────
extern bool ntpResynced3am;  // Blocks the 3 am resync from firing more than once per minute

// ── Reset button tracking ────────────────────────────────────
extern unsigned long resetPressedMs;    // millis() when button was first detected pressed
extern bool          resetHeldTracking; // true = actively counting down to factory reset

// ── Web server lifecycle ──────────────────────────────────────
extern bool serverStarted;  // Guards against double-calling server.begin()
