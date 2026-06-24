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
#define CAP_BOTTLE_MIN      17000   // Lower bound of water bottle detection range
#define CAP_BOTTLE_MAX      21000   // Upper bound of water bottle detection range
                                    // Below 17000 = empty, 17000-19000 = bottle, 45000+ = human (ignored)

#define PHOTO_PIN              21   // Digital-output photoresistor/LDR module (comparator DO pin)
                                    // HIGH = sufficient ambient light detected, LOW = dark

#define RESET_PIN              38   // Digital input, internal pull-up enabled
                                    // Active LOW: hold for RESET_HOLD_MS → factory reset
                                    // IMPORTANT: GPIO 33-37 are PSRAM interface pins on S2 Mini —
                                    // do NOT use them as user GPIO or PSRAM will crash

// ── Timing constants (all in milliseconds unless noted) ──────
#define BUZZER_ON_MS            500  // Buzzer stays ON for this long each alarm pulse
#define BUZZER_OFF_MS           500  // Buzzer stays OFF for this long between pulses
#define DISMISS_DEBOUNCE_MS    5000  // Both conditions must be held simultaneously this long to dismiss
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
#define PREF_KEY_SSID        "wifi_ssid"  // Active home WiFi SSID
#define PREF_KEY_PASS        "wifi_pass"  // Active home WiFi password
#define PREF_KEY_ALARM       "alarm_time" // Stored alarm time as "HH:MM" string
#define PREF_ALARM_DEFAULT   "07:00"      // Fallback alarm time if nothing is stored yet

// ── Saved network list (up to MAX_SAVED_NETWORKS entries) ─────
#define MAX_SAVED_NETWORKS   5
#define PREF_KEY_NET_COUNT   "net_count"  // int: number of saved networks
// Per-network keys are "net_ssid_N" and "net_pass_N" (N = 0..4)

// ── Cap sensor calibration (persisted thresholds) ─────────────
#define PREF_KEY_CAL_NOTHING "cal_nothing" // uint32: raw reading with nothing on pad
#define PREF_KEY_CAL_BOTTLE  "cal_bottle"  // uint32: raw reading with bottle on pad
#define PREF_KEY_CAL_HAND    "cal_hand"    // uint32: raw reading with hand on pad
#define PREF_KEY_CAP_MIN     "cap_min"     // uint32: computed lower threshold
#define PREF_KEY_CAP_MAX     "cap_max"     // uint32: computed upper threshold
