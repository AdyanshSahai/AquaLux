// ============================================================
//  webserver.cpp — Embedded HTML page and HTTP route handlers
//
//  Routes:
//    GET  /        — serve the test-panel UI from PROGMEM
//    GET  /status  — JSON snapshot of time + all sensor/buzzer states
//    POST /buzzer  — manually drive BUZZER_PIN for hardware testing
//    POST /save    — write new WiFi/alarm settings to NVS and reboot
// ============================================================

#include "webserver.h"

// ── buzzerTestState ───────────────────────────────────────────
// Tracks the manually-set buzzer state from the test UI.
// File-scoped so only this translation unit touches it.
// The alarm system (alarm.cpp) overrides the pin directly while alarmActive,
// but this flag reflects what the test UI last requested.
static bool buzzerTestState = false;

// ── INDEX_HTML ────────────────────────────────────────────────
// Test-panel UI stored in program flash (PROGMEM). Dark theme, mobile-first.
// Shows live sensor states and lets the user toggle the buzzer for hardware testing.
// Polls /status every 500 ms for responsive feedback.
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>AquaLux Test</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      background: #0d0d0d;
      color: #e0e0e0;
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      padding: 24px 16px 48px;
      max-width: 420px;
      margin: 0 auto;
    }

    h1 { font-size: 1.4rem; font-weight: 700; color: #fff; }
    .sub { font-size: 0.75rem; color: #555; margin-bottom: 24px; margin-top: 2px; }

    .time {
      font-size: 1.8rem;
      font-weight: 700;
      color: #fff;
      font-variant-numeric: tabular-nums;
      letter-spacing: 0.04em;
      margin-bottom: 20px;
    }

    .card {
      background: #141414;
      border: 1px solid #1f1f1f;
      border-radius: 14px;
      overflow: hidden;
      margin-bottom: 14px;
    }

    .row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 14px 16px;
      border-bottom: 1px solid #1a1a1a;
    }

    .row:last-child { border-bottom: none; }

    .label {
      font-size: 0.9rem;
      color: #ccc;
    }

    .badge {
      font-size: 0.7rem;
      font-weight: 600;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      padding: 3px 8px;
      border-radius: 20px;
      background: #222;
      color: #555;
      min-width: 42px;
      text-align: center;
      transition: background 0.2s, color 0.2s;
    }

    .badge.on  { background: #14532d; color: #4ade80; }
    .badge.off { background: #1f1f1f; color: #555;    }

    /* ── Toggle switch (used for the controllable buzzer row) ── */
    .toggle {
      position: relative;
      width: 44px;
      height: 24px;
      flex-shrink: 0;
    }

    .toggle input {
      opacity: 0;
      width: 0;
      height: 0;
      position: absolute;
    }

    .slider {
      position: absolute;
      inset: 0;
      background: #333;
      border-radius: 24px;
      cursor: pointer;
      transition: background 0.2s;
    }

    .slider::before {
      content: '';
      position: absolute;
      width: 18px;
      height: 18px;
      left: 3px;
      top: 3px;
      background: #888;
      border-radius: 50%;
      transition: transform 0.2s, background 0.2s;
    }

    .toggle input:checked + .slider { background: #166534; }
    .toggle input:checked + .slider::before {
      transform: translateX(20px);
      background: #4ade80;
    }
  </style>
</head>
<body>

  <h1>AquaLux</h1>
  <p class="sub">Hardware test panel</p>
  <div class="time" id="cur-time">--:--:--</div>

  <!-- Controllable outputs -->
  <div class="card">

    <!-- Buzzer: interactive toggle — actually drives BUZZER_PIN via /buzzer -->
    <div class="row">
      <span class="label">Buzzer</span>
      <label class="toggle">
        <input type="checkbox" id="toggle-buzzer" onchange="setBuzzer(this.checked)">
        <span class="slider"></span>
      </label>
    </div>

  </div>

  <!-- Live sensor states (read-only indicators) -->
  <div class="card">

    <div class="row">
      <span class="label">IR — bottle present</span>
      <span class="badge off" id="b-bottle">OFF</span>
    </div>

    <div class="row">
      <span class="label">Spring — bottle empty</span>
      <span class="badge off" id="b-spring">OFF</span>
    </div>

    <div class="row">
      <span class="label">Light / photo</span>
      <span class="badge off" id="b-light">OFF</span>
    </div>

    <div class="row">
      <span class="label">Alarm active</span>
      <span class="badge off" id="b-alarm">OFF</span>
    </div>

  </div>

  <script>
    // Update a read-only badge element based on a boolean state
    function badge(id, on) {
      const el = document.getElementById(id);
      if (!el) return;
      el.textContent = on ? 'ON' : 'OFF';
      el.className   = 'badge ' + (on ? 'on' : 'off');
    }

    // Poll /status every 500 ms — faster interval for hardware testing
    async function poll() {
      try {
        const r    = await fetch('/status');
        const data = await r.json();

        document.getElementById('cur-time').textContent = data.time || '--:--:--';

        // Sync the buzzer toggle to the last known hardware state
        // (alarm may have overridden it, so reflect the actual pin state)
        document.getElementById('toggle-buzzer').checked = !!data.buzzerOn;

        badge('b-bottle', data.bottlePresent);
        badge('b-spring', data.springExtended);
        badge('b-light',  data.lightDetected);
        badge('b-alarm',  data.alarmActive);

      } catch (_) {
        // Silently ignore — board may be briefly busy
      }
    }

    // POST to /buzzer with on=1 or on=0 to drive the pin directly
    async function setBuzzer(on) {
      try {
        await fetch('/buzzer', {
          method: 'POST',
          body: new URLSearchParams({ on: on ? '1' : '0' })
        });
      } catch (_) {}
    }

    poll();
    setInterval(poll, 500); // 500 ms for snappy feedback during testing
  </script>

</body>
</html>
)rawliteral";

// ── setupWebServer ────────────────────────────────────────────
// Registers all HTTP route handlers and starts listening on port 80.
// serverStarted flag prevents double-init if called more than once.
void setupWebServer() {
    if (serverStarted) return; // Already running — do nothing

    // ── GET / ─────────────────────────────────────────────────
    // Serve the test panel from PROGMEM
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", INDEX_HTML); // send_P streams directly from flash
    });

    // ── GET /status ───────────────────────────────────────────
    // JSON snapshot of time + all sensor/output states.
    // Polled every 500 ms by the test panel.
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String t = timeClient.isTimeSet() ? timeClient.getFormattedTime() : "--:--:--";
        String j = "{";
        j += "\"time\":"           "\"" + t                  + "\",";
        j += "\"alarmTime\":"      "\"" + alarmTimeStr        + "\",";
        j += "\"buzzerOn\":"           + String(buzzerTestState  ? "true" : "false") + ",";
        j += "\"alarmActive\":"        + String(alarmActive      ? "true" : "false") + ",";
        j += "\"lightDetected\":"      + String(lightDetected    ? "true" : "false") + ",";
        j += "\"bottlePresent\":"      + String(bottlePresent    ? "true" : "false") + ",";
        j += "\"springExtended\":"     + String(springExtended   ? "true" : "false");
        j += "}";
        request->send(200, "application/json", j);
    });

    // ── POST /buzzer ──────────────────────────────────────────
    // Manually drive BUZZER_PIN for hardware testing.
    // Body param: on=1 (buzzer on) or on=0 (buzzer off).
    // Note: if alarmActive is true, alarm.cpp will override the pin every ~50 ms.
    server.on("/buzzer", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool on = request->hasParam("on", true) &&
                  request->getParam("on", true)->value() == "1"; // true if on=1
        buzzerTestState = on;                   // Track the requested state
        digitalWrite(BUZZER_PIN, on ? HIGH : LOW); // Drive pin directly
        Serial.printf("[TEST] Buzzer manually set %s\n", on ? "ON" : "OFF");
        request->send(200, "text/plain", "OK");
    });

    // ── POST /save ────────────────────────────────────────────
    // Write new WiFi credentials and alarm time to NVS, then reboot.
    // Kept for completeness — not exposed in the test UI but reachable via curl/etc.
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        String newAlarm = request->hasParam("alarm", true)
                          ? request->getParam("alarm", true)->value() : alarmTimeStr;
        String newSSID  = request->hasParam("ssid",  true)
                          ? request->getParam("ssid",  true)->value() : storedSSID;
        String newPass  = request->hasParam("pass",  true)
                          ? request->getParam("pass",  true)->value() : storedPassword;

        preferences.begin(PREF_NAMESPACE, false);
        preferences.putString(PREF_KEY_SSID,  newSSID);
        preferences.putString(PREF_KEY_PASS,  newPass);
        preferences.putString(PREF_KEY_ALARM, newAlarm);
        preferences.end();

        Serial.printf("[SAVE] Stored SSID='%s'  Alarm='%s' — rebooting\n",
                      newSSID.c_str(), newAlarm.c_str());
        request->send(200, "text/plain", "OK");
        delay(300);
        ESP.restart();
    });

    server.begin();
    serverStarted = true;
    Serial.println("[SERVER] Web server running at " AP_GATEWAY_IP);
}
