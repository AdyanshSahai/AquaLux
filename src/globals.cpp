// ============================================================
//  globals.cpp — definitions of all shared global variables
//
//  Every translation unit that includes globals.h gets extern
//  declarations. This file holds the one real definition each.
//  Order matters for the NTPClient constructor: ntpUDP must be
//  fully constructed before timeClient, which takes it by ref.
// ============================================================

#include "globals.h"

// ── Library object instances ──────────────────────────────────
Preferences    preferences;                                                   // NVS handle
AsyncWebServer server(80);                                                    // HTTP on port 80
WiFiUDP        ntpUDP;                                                        // UDP socket; must be defined before timeClient
NTPClient      timeClient(ntpUDP, NTP_SERVER, NTP_UTC_OFFSET_SEC, NTP_AUTO_UPDATE_MS);
                           // NTP_AUTO_UPDATE_MS is 24 h — we control syncs manually via syncNTP();
                           // NTPClient still advances the clock locally using millis() between syncs

// ── Persisted configuration ───────────────────────────────────
String storedSSID     = "";              // Empty = no credentials in NVS yet
String storedPassword = "";
String alarmTimeStr   = PREF_ALARM_DEFAULT; // "07:00" until overwritten by loadPreferences()
int    alarmHour      = 7;               // Matches PREF_ALARM_DEFAULT hour
int    alarmMinute    = 0;               // Matches PREF_ALARM_DEFAULT minute

// ── Alarm runtime state ───────────────────────────────────────
bool          alarmActive          = false; // No alarm on boot
bool          buzzerOn             = false; // Buzzer starts off
unsigned long lastBuzzerToggle     = 0;     // Will be set by triggerAlarm()
bool          alarmFiredThisMinute = false; // No alarm has fired yet

// ── Dismiss debounce state ────────────────────────────────────
bool          dismissInProgress = false;
unsigned long dismissStartMs    = 0;

// ── Sensor readings ───────────────────────────────────────────
bool capDetected   = false; // Updated by readSensors()
bool lightDetected = false;

// ── Daily NTP resync tracking ─────────────────────────────────
bool ntpResynced3am = false;

// ── Reset button tracking ────────────────────────────────────
unsigned long resetPressedMs    = 0;
bool          resetHeldTracking = false;

// ── Web server lifecycle ──────────────────────────────────────
bool serverStarted = false;
