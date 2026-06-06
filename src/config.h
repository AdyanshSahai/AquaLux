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
#define IR_PRESENCE_PIN         15  // ← CHANGE ME — digital input, no internal pull-up needed
                                    // IR obstacle-detection module has its own pull-up resistor
                                    // HIGH = IR reflection detected = bottle is on the platform

#define SPRING_SWITCH_PIN       8   // ← CHANGE ME — digital input, internal pull-DOWN enabled
                                    // CUSTOM SPRING SWITCH (homemade card + 3D-printed spring)
                                    // Wired: switch between SPRING_SWITCH_PIN and 3.3V
                                    // LOW  = switch open  = spring compressed = bottle present
                                    // HIGH = switch closed to 3.3V = spring extended = bottle empty/removed

#define RESET_PIN               38   // ← CHANGE ME — digital input, internal pull-up enabled
                                    // Active LOW: hold for RESET_HOLD_MS → factory reset
                                    // IMPORTANT: GPIO 33-37 are PSRAM interface pins on S2 Mini —
                                    // do NOT use them as user GPIO or PSRAM will crash

// ── COMMENTED OUT: future photoresistor on analog pin ────────
#define PHOTO_PIN             7   // Analog GPIO for photoresistor voltage-divider
                                  // Wiring: 3.3 V → photoresistor → PHOTO_PIN → 10 kΩ → GND
                                  // When lit  : photoresistor resistance drops → voltage rises → ADC high
                                  // When dark : photoresistor resistance high  → voltage falls → ADC low
#define LIGHT_THRESHOLD    7900   // ESP32-S2 has a 13-bit ADC (0–8191, not 0–4095)
                                  // Calibrated: lights OFF + curtains open ≈ 7608, lights ON ≈ 8191
                                  // 7900 sits cleanly between the two — raise if false-triggers on bright days

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
