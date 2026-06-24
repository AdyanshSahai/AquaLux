#pragma once
// ============================================================
//  wifi_ntp.h — WiFi credential management and NTP sync
// ============================================================

#include "globals.h"

// Splits "HH:MM" string into alarmHour and alarmMinute globals.
// Falls back to 07:00 on malformed input.
void parseAlarmTime(const String &timeStr);

// Reads WiFi credentials and alarm time from NVS into globals.
// Returns true if a non-empty SSID was found (credentials available).
bool loadPreferences();

// Briefly joins home WiFi, syncs NTP, disconnects, restores AP mode.
// Returns true if the time was successfully updated.
bool syncNTP();

// Time helpers — use NTP if synced, fall back to manually-set time + millis() drift.
bool   isAnyTimeSet();
int    getCurrentHour();
int    getCurrentMinute();
String getCurrentTimeStr();
