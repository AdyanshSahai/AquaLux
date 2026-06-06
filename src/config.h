#pragma once
// ============================================================
//  config.h — Hardware pin assignments and compile-time constants
//
//  ALL magic numbers live here. No numeric literals anywhere else.
//  *** CHANGE THE GPIO NUMBERS to match your actual wiring. ***
// ============================================================

// ── Output pins ──────────────────────────────────────────────
#define BUZZER_PIN             14   // ← CHANGE ME — digital output
                                    // HIGH drives 2N2222 base via series resistor
                                    // → transistor saturates → active buzzer sounds

// ── Permanent digital input pins ─────────────────────────────
#define IR_PRESENCE_PIN         7   // ← CHANGE ME — digital input, no internal pull-up needed
                                    // IR obstacle-detection module has its own pull-up resistor
                                    // HIGH = IR reflection detected = bottle is on the platform

#define SPRING_SWITCH_PIN       8   // ← CHANGE ME — digital input, internal pull-up enabled
                                    // CUSTOM SPRING SWITCH (homemade card + 3D-printed spring)
                                    // LOW  = bottle weight compresses spring (bottle present, possibly full)
                                    // HIGH = spring extends when bottle is empty / removed

#define RESET_PIN               33   // ← CHANGE ME — digital input, internal pull-up enabled
                                    // Active LOW: hold for RESET_HOLD_MS → factory reset

// ── COMMENTED OUT: future photoresistor on analog pin ────────
#define PHOTO_PIN             15   // Analog GPIO for photoresistor voltage-divider
                                  // Wiring: 3.3 V → photoresistor → PHOTO_PIN → 10 kΩ → GND
                                  // When lit  : photoresistor resistance drops → voltage rises → ADC high
                                  // When dark : photoresistor resistance high  → voltage falls → ADC low
#define LIGHT_THRESHOLD    2000   // ADC dismissal threshold (12-bit range: 0 dark → 4095 bright)
                                  // Tune: measure ADC with room lights on, set to ~80% of that reading

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
#define NTP_UTC_OFFSET_SEC    0             // UTC offset in seconds; adjust for your timezone
                                            // Examples: IST = 19800, EST = -18000, CET = 3600

// ── Preferences (NVS) keys ────────────────────────────────────
#define PREF_NAMESPACE       "aqualux"    // NVS namespace that groups all AquaLux keys
#define PREF_KEY_SSID        "wifi_ssid"  // Stored home WiFi SSID
#define PREF_KEY_PASS        "wifi_pass"  // Stored home WiFi password
#define PREF_KEY_ALARM       "alarm_time" // Stored alarm time as "HH:MM" string
#define PREF_ALARM_DEFAULT   "07:00"      // Fallback alarm time if nothing is stored yet
