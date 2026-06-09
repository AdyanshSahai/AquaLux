#pragma once
// ============================================================
//  config.h — Hardware pin assignments and compile-time constants
//
//  ALL magic numbers live here. No numeric literals anywhere else.
//  *** CHANGE THE GPIO NUMBERS to match your actual wiring. ***
// ============================================================

// ── Output pins ──────────────────────────────────────────────
#define BUZZER_PIN             40   // Digital output → 2N2222 base via series resistor
                                    // HIGH → transistor saturates → active buzzer sounds

// ── Digital input pins ────────────────────────────────────────
#define CAP_SENSOR_PIN          5   // ESP32-S2 touch pin T5; read with touchRead()
                                    // Lower value = more capacitance = touch/presence detected
#define CAP_THRESHOLD       40000   // Readings below this count as "detected" — tune via Dev mode

#define PHOTO_PIN              21   // Digital-output photoresistor/LDR module (comparator DO pin)
                                    // HIGH = sufficient ambient light detected, LOW = dark

#define RESET_PIN              38   // Digital input, internal pull-up enabled
                                    // Active LOW: hold for RESET_HOLD_MS → factory reset
                                    // IMPORTANT: GPIO 33-37 are PSRAM interface pins on S2 Mini —
                                    // do NOT use them as user GPIO or PSRAM will crash

// ── Timing constants (all in milliseconds unless noted) ──────
#define BUZZER_ON_MS            500  // Buzzer stays ON for this long each alarm pulse
#define BUZZER_OFF_MS           500  // Buzzer stays OFF for this long between pulses
#define DISMISS_DEBOUNCE_MS    2000  // All three sensors must be satisfied continuously this long to dismiss
#define RESET_HOLD_MS          3000  // Hold reset button this many ms to trigger factory reset
#define RESET_BEEP_MS           200  // Length of confirmation beep after factory reset wipe
#define LOOP_IDLE_DELAY_MS      100  // Yield delay each loop iteration when alarm is inactive
#define LOOP_ALARM_DELAY_MS      50  // Shorter yield when alarm is active (faster buzzer pulse response)

// ── NTP / scheduling constants ────────────────────────────────
#define NTP_RESYNC_HOUR           3  // 24-hour clock hour for daily NTP resync
#define NTP_RESYNC_MINUTE         0  // Minute within that hour
#define NTP_CONNECT_TIMEOUT_MS 5000  // Max ms to wait for home-WiFi connection during NTP sync
#define NTP_AUTO_UPDATE_MS    86400000UL // 24 h internal NTPClient update interval (we manage syncs manually)

// ── Network constants ─────────────────────────────────────────
#define AP_SSID              "AquaLux"      // Broadcast SSID for the device's own AP (no password)
#define AP_GATEWAY_IP        "192.168.4.1"  // Default ESP32 softAP gateway IP (informational)
#define NTP_SERVER           "pool.ntp.org" // Global NTP pool; replace with regional pool if preferred
#define NTP_UTC_OFFSET_SEC    3600          // UTC offset in seconds; adjust for your timezone
                                            // Examples: IST = 19800, EST = -18000, CET = 3600

// ── Preferences (NVS) keys ────────────────────────────────────
#define PREF_NAMESPACE       "aqualux"    // NVS namespace that groups all AquaLux keys
#define PREF_KEY_SSID        "wifi_ssid"  // Stored home WiFi SSID
#define PREF_KEY_PASS        "wifi_pass"  // Stored home WiFi password
#define PREF_KEY_ALARM       "alarm_time" // Stored alarm time as "HH:MM" string
#define PREF_ALARM_DEFAULT   "07:00"      // Fallback alarm time if nothing is stored yet
