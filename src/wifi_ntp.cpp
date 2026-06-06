// ============================================================
//  wifi_ntp.cpp — WiFi credential management and NTP sync
// ============================================================

#include "wifi_ntp.h"

// ── parseAlarmTime ────────────────────────────────────────────
// Splits a "HH:MM" string into alarmHour and alarmMinute integers.
// Called whenever a new alarm string is loaded from Preferences.
void parseAlarmTime(const String &timeStr) {
    int colonIdx = timeStr.indexOf(':');  // Find the separator between hours and minutes
    if (colonIdx < 1) {                   // Guard: malformed if colon is missing or at index 0
        Serial.println("[CONFIG] Malformed alarm time string — falling back to 07:00");
        alarmHour   = 7;
        alarmMinute = 0;
        return;
    }
    alarmHour   = timeStr.substring(0, colonIdx).toInt();  // Everything before ':' → integer
    alarmMinute = timeStr.substring(colonIdx + 1).toInt(); // Everything after  ':' → integer
    Serial.printf("[CONFIG] Alarm time set: %02d:%02d\n", alarmHour, alarmMinute);
}

// ── loadPreferences ───────────────────────────────────────────
// Reads NVS for WiFi credentials and alarm time, updates global config variables.
// Returns true if a non-empty SSID was found (credentials are available for NTP sync).
bool loadPreferences() {
    preferences.begin(PREF_NAMESPACE, true);  // Open read-only; false = read-write

    storedSSID     = preferences.getString(PREF_KEY_SSID,  "");                // "" if key doesn't exist
    storedPassword = preferences.getString(PREF_KEY_PASS,  "");
    alarmTimeStr   = preferences.getString(PREF_KEY_ALARM, PREF_ALARM_DEFAULT); // Default if never set

    preferences.end(); // Release the NVS handle; always pair begin/end to avoid handle leaks

    parseAlarmTime(alarmTimeStr); // Populate alarmHour / alarmMinute from the loaded string

    bool hasCreds = (storedSSID.length() > 0); // Only true when an SSID has actually been stored
    Serial.printf("[PREFS] SSID='%s'  Alarm='%s'  Creds=%s\n",
                  storedSSID.c_str(), alarmTimeStr.c_str(), hasCreds ? "YES" : "NO");
    return hasCreds;
}

// ── syncNTP ───────────────────────────────────────────────────
// Briefly switches to WIFI_AP_STA, connects home WiFi, syncs NTP, then disconnects.
// Restores WIFI_AP mode and re-asserts the softAP when done.
// The re-assert is needed because a mode switch can reset the AP config on the S2.
// Returns true if the time was successfully updated.
bool syncNTP() {
    if (storedSSID.length() == 0) {
        Serial.println("[NTP] No SSID — skipping sync");
        return false;
    }
    Serial.printf("[NTP] Connecting to '%s' for NTP sync...\n", storedSSID.c_str());

    WiFi.mode(WIFI_AP_STA);  // AP_STA keeps the existing softAP running while STA connects
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str()); // Kick off STA association

    unsigned long connectStart = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - connectStart < NTP_CONNECT_TIMEOUT_MS) {
        delay(100); // 10 Hz poll — yields to other FreeRTOS tasks without busy-spinning
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] Connection timed out — sync skipped");
        WiFi.disconnect(true);  // Clean up the failed STA attempt
        WiFi.mode(WIFI_AP);     // Back to AP-only
        WiFi.softAP(AP_SSID);  // Re-assert softAP (mode switch may clear it on S2)
        return false;
    }

    Serial.println("[NTP] Connected — requesting time...");
    timeClient.forceUpdate(); // Block until NTP UDP response arrives (or internal timeout)

    bool synced = timeClient.isTimeSet(); // Verify we got a valid epoch timestamp
    if (synced) {
        Serial.printf("[NTP] Synced OK — current time: %s\n",
                      timeClient.getFormattedTime().c_str());
    } else {
        Serial.println("[NTP] NTP response not received — clock unchanged");
    }

    WiFi.disconnect(true);  // Drop home WiFi (saves power; AP clients don't need internet)
    WiFi.mode(WIFI_AP);     // Return to AP-only mode
    WiFi.softAP(AP_SSID);  // Re-assert softAP so existing clients can still reach 192.168.4.1
    Serial.println("[NTP] Home WiFi disconnected — AP restored");
    return synced;
}
