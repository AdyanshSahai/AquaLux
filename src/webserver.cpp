// ============================================================
//  webserver.cpp — Embedded HTML page and HTTP route handlers
//
//  Routes:
//    GET  /        — serve the full single-page UI from PROGMEM
//    GET  /status  — JSON snapshot of time + all sensor/alarm states
//    POST /save    — write new settings to NVS and reboot
// ============================================================

#include "webserver.h"

// ── INDEX_HTML ────────────────────────────────────────────────
// Complete single-page configuration UI stored in program flash (PROGMEM).
// Dark theme, mobile-first. Polls /status every 2 s for live data.
// static = file-scoped; no other translation unit needs to see this symbol.
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>AquaLux</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      background: #0d0d0d;
      color: #e0e0e0;
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      padding: 20px 16px 48px;
      max-width: 480px;
      margin: 0 auto;
    }

    h1 { font-size: 1.7rem; font-weight: 700; color: #fff; margin-bottom: 3px; }
    .sub { font-size: 0.78rem; color: #555; margin-bottom: 24px; }

    .card {
      background: #161616;
      border: 1px solid #212121;
      border-radius: 14px;
      padding: 18px 16px;
      margin-bottom: 14px;
    }

    .card-title {
      font-size: 0.68rem;
      text-transform: uppercase;
      letter-spacing: 0.1em;
      color: #4a4a4a;
      margin-bottom: 14px;
    }

    .time-big {
      font-size: 2.5rem;
      font-weight: 700;
      text-align: center;
      color: #fff;
      letter-spacing: 0.06em;
      font-variant-numeric: tabular-nums;
    }

    .alarm-note {
      text-align: center;
      font-size: 0.78rem;
      color: #666;
      margin-top: 6px;
    }

    .alarm-note.ringing { color: #f59e0b; font-weight: 600; }

    .sensor-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 9px 0;
      border-bottom: 1px solid #1c1c1c;
      font-size: 0.88rem;
    }

    .sensor-row:last-child { border-bottom: none; }

    .dot {
      width: 10px; height: 10px;
      border-radius: 50%;
      background: #2a2a2a;
      flex-shrink: 0;
      transition: background 0.2s;
    }

    .dot.on  { background: #22c55e; }
    .dot.off { background: #3a3a3a; }

    label {
      display: block;
      font-size: 0.78rem;
      color: #666;
      margin-bottom: 5px;
      margin-top: 13px;
    }

    label:first-of-type { margin-top: 0; }

    input[type="text"],
    input[type="password"],
    input[type="time"] {
      display: block; width: 100%;
      background: #0d0d0d;
      border: 1px solid #252525;
      border-radius: 9px;
      color: #e0e0e0;
      font-size: 0.95rem;
      padding: 10px 12px;
      outline: none;
      transition: border-color 0.15s;
      -webkit-appearance: none;
    }

    input:focus { border-color: #3b82f6; }

    .btn {
      display: block; width: 100%;
      margin-top: 18px; padding: 13px;
      background: #1d4ed8;
      color: #fff; border: none;
      border-radius: 10px;
      font-size: 0.95rem; font-weight: 600;
      cursor: pointer;
      transition: background 0.15s;
    }

    .btn:active { background: #1e40af; }

    #msg {
      text-align: center;
      font-size: 0.8rem;
      color: #22c55e;
      min-height: 20px;
      margin-top: 10px;
    }
  </style>
</head>
<body>

  <h1>AquaLux</h1>
  <p class="sub">Hydration alarm &mdash; connected to AP</p>

  <!-- Clock + alarm status -->
  <div class="card">
    <div class="card-title">Current Time</div>
    <div class="time-big" id="cur-time">--:--:--</div>
    <div class="alarm-note" id="alarm-note">Loading...</div>
  </div>

  <!-- Live sensor states -->
  <div class="card">
    <div class="card-title">Sensor Status</div>
    <div class="sensor-row">
      <span>Light / wake detected</span>
      <div class="dot" id="d-light"></div>
    </div>
    <div class="sensor-row">
      <span>Bottle present (IR)</span>
      <div class="dot" id="d-bottle"></div>
    </div>
    <div class="sensor-row">
      <span>Bottle emptied (spring)</span>
      <div class="dot" id="d-spring"></div>
    </div>
    <div class="sensor-row">
      <span>Alarm ringing</span>
      <div class="dot" id="d-alarm"></div>
    </div>
  </div>

  <!-- Settings form -->
  <div class="card">
    <div class="card-title">Settings</div>
    <label for="f-alarm">Alarm Time</label>
    <input type="time" id="f-alarm" value="07:00">
    <label for="f-ssid">Home WiFi Network</label>
    <input type="text"     id="f-ssid" placeholder="Network name" autocomplete="off" spellcheck="false">
    <label for="f-pass">WiFi Password</label>
    <input type="password" id="f-pass" placeholder="Password"     autocomplete="off">
    <button class="btn" onclick="save()">Save &amp; Reboot</button>
    <div id="msg"></div>
  </div>

  <script>
    let alarmPrefilled = false; // Only auto-fill alarm input from /status once per page load

    async function poll() {
      try {
        const r    = await fetch('/status');
        const data = await r.json();

        // Update the clock display
        document.getElementById('cur-time').textContent = data.time || '--:--:--';

        // Update the alarm state note below the clock
        const note = document.getElementById('alarm-note');
        if (data.alarmActive) {
          note.textContent = 'Alarm is ringing now!';
          note.className   = 'alarm-note ringing';
        } else {
          note.textContent = 'Next alarm: ' + (data.alarmTime || '--:--');
          note.className   = 'alarm-note';
        }

        // Update sensor indicator dots
        setDot('d-light',  data.lightDetected);
        setDot('d-bottle', data.bottlePresent);
        setDot('d-spring', data.springExtended);
        setDot('d-alarm',  data.alarmActive);

        // Pre-fill the alarm input with the stored value — only once so user edits aren't overwritten
        if (!alarmPrefilled && data.alarmTime) {
          document.getElementById('f-alarm').value = data.alarmTime;
          alarmPrefilled = true;
        }

      } catch (_) {
        // Silently ignore — device may be briefly busy or rebooting after save
      }
    }

    function setDot(id, active) {
      const el = document.getElementById(id);
      if (el) el.className = 'dot ' + (active ? 'on' : 'off');
    }

    async function save() {
      const params = new URLSearchParams({
        alarm: document.getElementById('f-alarm').value,
        ssid:  document.getElementById('f-ssid').value,
        pass:  document.getElementById('f-pass').value,
      });
      const msg = document.getElementById('msg');
      try {
        const r = await fetch('/save', { method: 'POST', body: params });
        if (r.ok) {
          msg.textContent = 'Saved! Device is rebooting...';
          msg.style.color = '#22c55e';
        } else {
          msg.textContent = 'Server error — try again';
          msg.style.color = '#ef4444';
        }
      } catch (_) {
        msg.textContent = 'Could not reach device';
        msg.style.color = '#ef4444';
      }
    }

    poll();                    // First poll immediately on page load
    setInterval(poll, 2000);   // Then every 2 seconds
  </script>
</body>
</html>
)rawliteral";

// ── setupWebServer ────────────────────────────────────────────
// Registers all HTTP route handlers and starts listening on port 80.
// The serverStarted flag prevents accidental double-initialisation
// (e.g. syncNTP() calls WiFi.softAP again, which could prompt a second call here).
void setupWebServer() {
    if (serverStarted) return; // Already running — do nothing

    // ── GET / ─────────────────────────────────────────────────
    // Serve the embedded HTML page directly from PROGMEM (no filesystem required)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", INDEX_HTML); // send_P streams from flash, not RAM
    });

    // ── GET /status ───────────────────────────────────────────
    // Returns a JSON snapshot of the current time and all sensor/alarm states.
    // The web page polls this endpoint every 2 seconds to update the UI.
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Build a flat JSON string manually — avoids pulling in ArduinoJson for such a simple payload
        String t = timeClient.isTimeSet() ? timeClient.getFormattedTime() : "--:--:--";
        String j = "{";
        j += "\"time\":"           "\"" + t             + "\",";
        j += "\"alarmTime\":"      "\"" + alarmTimeStr  + "\",";
        j += "\"alarmActive\":"        + String(alarmActive     ? "true" : "false") + ",";
        j += "\"lightDetected\":"      + String(lightDetected   ? "true" : "false") + ",";
        j += "\"bottlePresent\":"      + String(bottlePresent   ? "true" : "false") + ",";
        j += "\"springExtended\":"     + String(springExtended  ? "true" : "false");
        j += "}";
        request->send(200, "application/json", j);
    });

    // ── POST /save ────────────────────────────────────────────
    // Receives new settings from the form, writes them to NVS, and reboots.
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        // hasParam(key, true) checks the POST body; second arg true = body params, not URL params
        String newAlarm = request->hasParam("alarm", true)
                          ? request->getParam("alarm", true)->value() : alarmTimeStr;
        String newSSID  = request->hasParam("ssid",  true)
                          ? request->getParam("ssid",  true)->value() : storedSSID;
        String newPass  = request->hasParam("pass",  true)
                          ? request->getParam("pass",  true)->value() : storedPassword;

        preferences.begin(PREF_NAMESPACE, false); // Open NVS in read-write mode
        preferences.putString(PREF_KEY_SSID,  newSSID);
        preferences.putString(PREF_KEY_PASS,  newPass);
        preferences.putString(PREF_KEY_ALARM, newAlarm);
        preferences.end(); // Flush to flash and close handle

        Serial.printf("[SAVE] Stored SSID='%s'  Alarm='%s' — rebooting\n",
                      newSSID.c_str(), newAlarm.c_str());

        request->send(200, "text/plain", "OK"); // Send response before rebooting so client sees confirmation
        delay(300);      // Give AsyncTCP time to flush the response packet to the client
        ESP.restart();   // Hard reboot — fresh boot will load the newly saved settings
    });

    server.begin();       // Start accepting connections on port 80
    serverStarted = true; // Mark started so a second call is a no-op
    Serial.println("[SERVER] Web server running at " AP_GATEWAY_IP);
}
