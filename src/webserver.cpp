// ============================================================
//  webserver.cpp — Embedded HTML UI and HTTP route handlers
//
//  Routes:
//    GET  /         — serve the three-mode UI from PROGMEM
//    GET  /status   — JSON snapshot of time + sensor/buzzer/alarm states
//    POST /setalarm — update alarm time in NVS + globals (no reboot)
//    POST /buzzer   — manually drive BUZZER_PIN (dev mode)
//    POST /save     — write WiFi credentials + current alarm to NVS → reboot
// ============================================================

#include "webserver.h"
#include "wifi_ntp.h"   // parseAlarmTime()

static bool buzzerTestState = false;

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
      min-height: 100vh;
    }

    /* ── Top bar ─────────────────────────────────────────────── */
    .topbar {
      position: relative;
      display: flex;
      align-items: center;
      height: 52px;
      padding: 0 16px;
      background: #141414;
      border-bottom: 1px solid #1f1f1f;
      z-index: 100;
    }

    .topbar-title {
      position: absolute;
      left: 50%;
      transform: translateX(-50%);
      font-size: 1rem;
      font-weight: 700;
      color: #fff;
      letter-spacing: 0.08em;
    }

    /* ── Hamburger button ────────────────────────────────────── */
    .ham-btn {
      background: none;
      border: none;
      cursor: pointer;
      padding: 6px;
      display: flex;
      flex-direction: column;
      gap: 5px;
    }

    .ham-btn span {
      display: block;
      width: 22px;
      height: 2px;
      background: #ccc;
      border-radius: 2px;
      transition: background 0.2s;
    }

    .ham-btn:hover span { background: #fff; }

    /* ── Dropdown menu ───────────────────────────────────────── */
    .dropdown {
      position: absolute;
      top: 52px;
      left: 0;
      width: 180px;
      background: #1a1a1a;
      border: 1px solid #2a2a2a;
      border-radius: 0 0 10px 10px;
      z-index: 200;
      overflow: hidden;
    }

    .dropdown.hidden { display: none; }

    .dropdown a {
      display: block;
      padding: 13px 18px;
      font-size: 0.9rem;
      color: #ccc;
      cursor: pointer;
      border-bottom: 1px solid #222;
      transition: background 0.15s, color 0.15s;
      user-select: none;
    }

    .dropdown a:last-child { border-bottom: none; }
    .dropdown a:hover { background: #222; color: #fff; }
    .dropdown a.active { color: #4ade80; }

    /* ── Main content ────────────────────────────────────────── */
    .content {
      padding: 28px 16px 48px;
      max-width: 420px;
      margin: 0 auto;
    }

    .mode-panel { display: none; }
    .mode-panel.active { display: block; }

    /* ── Section headings ────────────────────────────────────── */
    .panel-title {
      font-size: 1.1rem;
      font-weight: 700;
      color: #fff;
      margin-bottom: 6px;
    }

    .panel-sub {
      font-size: 0.75rem;
      color: #555;
      margin-bottom: 24px;
    }

    /* ── Cards ───────────────────────────────────────────────── */
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

    .label { font-size: 0.9rem; color: #ccc; }

    /* ── Form inputs ─────────────────────────────────────────── */
    .field { margin-bottom: 16px; }

    .field label {
      display: block;
      font-size: 0.78rem;
      color: #888;
      margin-bottom: 6px;
      letter-spacing: 0.04em;
      text-transform: uppercase;
    }

    .field input {
      width: 100%;
      background: #141414;
      border: 1px solid #2a2a2a;
      border-radius: 10px;
      color: #fff;
      font-size: 1rem;
      padding: 11px 14px;
      outline: none;
      transition: border-color 0.2s;
      -webkit-appearance: none;
    }

    .field input:focus { border-color: #4ade80; }
    .field input:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }

    /* ── Buttons ─────────────────────────────────────────────── */
    .btn {
      width: 100%;
      padding: 13px;
      border: none;
      border-radius: 10px;
      font-size: 0.95rem;
      font-weight: 600;
      cursor: pointer;
      transition: opacity 0.2s;
    }

    .btn:disabled { opacity: 0.4; cursor: not-allowed; }
    .btn-green { background: #166534; color: #4ade80; }
    .btn-green:hover:not(:disabled) { opacity: 0.85; }
    .btn-blue  { background: #1e3a5f; color: #60a5fa; }
    .btn-blue:hover:not(:disabled)  { opacity: 0.85; }

    /* ── Status light (alarm mode) ───────────────────────────── */
    .status-wrap {
      display: flex;
      flex-direction: column;
      align-items: center;
      margin-top: 28px;
      gap: 10px;
    }

    .status-dot {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: #166534;
      box-shadow: 0 0 12px #4ade8044;
      transition: background 0.3s, box-shadow 0.3s;
    }

    .status-dot.alarming {
      background: #991b1b;
      box-shadow: 0 0 18px #ef444466;
      animation: pulse-red 1s ease-in-out infinite;
    }

    @keyframes pulse-red {
      0%, 100% { box-shadow: 0 0 12px #ef444444; }
      50%       { box-shadow: 0 0 28px #ef4444aa; }
    }

    .status-label {
      font-size: 0.8rem;
      color: #666;
    }

    /* ── Sensor badges ───────────────────────────────────────── */
    .badge {
      font-size: 0.7rem;
      font-weight: 600;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      padding: 3px 8px;
      border-radius: 20px;
      background: #1f1f1f;
      color: #555;
      min-width: 40px;
      text-align: center;
      transition: background 0.2s, color 0.2s;
    }

    .badge.on  { background: #14532d; color: #4ade80; }
    .badge.off { background: #1f1f1f; color: #555; }

    /* ── Time display (dev mode) ─────────────────────────────── */
    .time-display {
      font-size: 2rem;
      font-weight: 700;
      color: #fff;
      font-variant-numeric: tabular-nums;
      letter-spacing: 0.04em;
      margin-bottom: 20px;
    }

    /* ── Toggle switch ───────────────────────────────────────── */
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

    /* ── Setup message ───────────────────────────────────────── */
    .msg {
      font-size: 0.8rem;
      margin-top: 12px;
      padding: 10px 14px;
      border-radius: 8px;
      display: none;
    }

    .msg.ok  { background: #14532d44; color: #4ade80; display: block; }
    .msg.err { background: #7f1d1d44; color: #f87171; display: block; }

    /* ── WiFi scan results ───────────────────────────────────── */
    .scan-list {
      margin-top: 8px;
      border: 1px solid #2a2a2a;
      border-radius: 10px;
      overflow: hidden;
      display: none;
    }

    .scan-list.visible { display: block; }

    .scan-item {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 11px 14px;
      border-bottom: 1px solid #1a1a1a;
      cursor: pointer;
      transition: background 0.15s;
    }

    .scan-item:last-child { border-bottom: none; }
    .scan-item:hover { background: #1e1e1e; }

    .scan-item .ssid-name { font-size: 0.9rem; color: #ddd; }
    .scan-item .rssi {
      font-size: 0.75rem;
      color: #555;
    }
  </style>
</head>
<body>

  <!-- ── Top bar ──────────────────────────────────────────── -->
  <header class="topbar">
    <button class="ham-btn" id="ham-btn" aria-label="Menu">
      <span></span><span></span><span></span>
    </button>
    <span class="topbar-title">AquaLux</span>
  </header>

  <!-- ── Dropdown nav ─────────────────────────────────────── -->
  <nav class="dropdown hidden" id="dropdown">
    <a id="nav-alarm" class="active" onclick="setMode('alarm')">Alarm</a>
    <a id="nav-setup"               onclick="setMode('setup')">Setup</a>
    <a id="nav-dev"                 onclick="setMode('dev')">Dev</a>
  </nav>

  <!-- ── Content ──────────────────────────────────────────── -->
  <div class="content">

    <!-- ── ALARM MODE ──────────────────────────────────────── -->
    <section class="mode-panel active" id="panel-alarm">
      <p class="panel-title">Alarm</p>
      <p class="panel-sub">Set your daily hydration reminder</p>

      <div class="card">
        <div class="row" style="flex-direction:column;align-items:flex-start;gap:10px;">
          <div class="field" style="width:100%;margin-bottom:0;">
            <label>Alarm Time</label>
            <input type="time" id="alarm-input" value="07:00">
          </div>
          <button class="btn btn-green" id="save-btn" onclick="saveAlarm()">Save</button>
        </div>
      </div>

      <div class="status-wrap">
        <div class="status-dot" id="status-dot"></div>
        <span class="status-label" id="status-label">Not ringing</span>
      </div>
    </section>

    <!-- ── SETUP MODE ──────────────────────────────────────── -->
    <section class="mode-panel" id="panel-setup">
      <p class="panel-title">WiFi Setup</p>
      <p class="panel-sub">Connect to home WiFi for automatic time sync</p>

      <div class="card">
        <div class="row" style="flex-direction:column;align-items:flex-start;gap:4px;">
          <div class="field" style="width:100%;">
            <label>Network Name (SSID)</label>
            <div style="display:flex;gap:8px;">
              <input type="text" id="ssid-input" placeholder="Your WiFi name" autocomplete="off" style="flex:1;">
              <button class="btn btn-blue" id="scan-btn" onclick="scanWifi()" style="width:auto;padding:11px 14px;white-space:nowrap;">Scan</button>
            </div>
            <div class="scan-list" id="scan-list"></div>
          </div>
          <div class="field" style="width:100%;margin-bottom:0;">
            <label>Password</label>
            <input type="password" id="pass-input" placeholder="WiFi password" autocomplete="off">
          </div>
        </div>
        <div class="row">
          <button class="btn btn-blue" id="connect-btn" onclick="saveWifi()">Connect &amp; Sync</button>
        </div>
      </div>

      <p class="msg" id="setup-msg"></p>

      <div class="card" style="margin-top:14px;">
        <div class="row">
          <span class="label">Current time</span>
          <span style="font-size:1rem;font-weight:700;color:#fff;font-variant-numeric:tabular-nums;" id="setup-time">--:--:--</span>
        </div>
        <div class="row">
          <button class="btn btn-blue" id="sync-btn" onclick="syncTime()">Sync Time Now</button>
        </div>
      </div>
    </section>

    <!-- ── DEV MODE ────────────────────────────────────────── -->
    <section class="mode-panel" id="panel-dev">
      <p class="panel-title">Dev</p>
      <p class="panel-sub">Live sensor readings and buzzer control</p>

      <div class="time-display" id="cur-time">--:--:--</div>

      <div class="card">
        <div class="row">
          <span class="label">Capacitive sensor</span>
          <span style="display:flex;align-items:center;gap:8px;">
            <span style="font-size:0.85rem;color:#aaa;font-variant-numeric:tabular-nums;" id="cap-raw">--</span>
            <span class="badge off" id="b-cap">OFF</span>
          </span>
        </div>
        <div class="row">
          <span class="label">Photoresistor</span>
          <span class="badge off" id="b-light">OFF</span>
        </div>
        <div class="row">
          <span class="label">Alarm active</span>
          <span class="badge off" id="b-alarm">OFF</span>
        </div>
      </div>

      <div class="card">
        <div class="row">
          <span class="label">Buzzer</span>
          <label class="toggle">
            <input type="checkbox" id="toggle-buzzer" onchange="setBuzzer(this.checked)">
            <span class="slider"></span>
          </label>
        </div>
      </div>
    </section>

  </div><!-- .content -->

  <script>
    // ── Mode switching ────────────────────────────────────────
    function setMode(mode) {
      document.querySelectorAll('.mode-panel').forEach(p => p.classList.remove('active'));
      document.querySelectorAll('.dropdown a').forEach(a => a.classList.remove('active'));
      document.getElementById('panel-' + mode).classList.add('active');
      document.getElementById('nav-' + mode).classList.add('active');
      document.getElementById('dropdown').classList.add('hidden');
    }

    document.getElementById('ham-btn').addEventListener('click', function(e) {
      e.stopPropagation();
      document.getElementById('dropdown').classList.toggle('hidden');
    });

    document.addEventListener('click', function(e) {
      const dd = document.getElementById('dropdown');
      if (!dd.classList.contains('hidden') &&
          !dd.contains(e.target) &&
          e.target !== document.getElementById('ham-btn')) {
        dd.classList.add('hidden');
      }
    });

    // ── Alarm save ────────────────────────────────────────────
    async function saveAlarm() {
      const val = document.getElementById('alarm-input').value;
      if (!val) return;
      const btn = document.getElementById('save-btn');
      btn.disabled = true;
      try {
        const r = await fetch('/setalarm', {
          method: 'POST',
          body: new URLSearchParams({ alarm: val })
        });
        btn.textContent = r.ok ? 'Saved!' : 'Error';
        setTimeout(() => { btn.textContent = 'Save'; btn.disabled = false; }, 2000);
      } catch(_) {
        btn.textContent = 'Error';
        setTimeout(() => { btn.textContent = 'Save'; btn.disabled = false; }, 2000);
      }
    }

    // ── WiFi save (reboots device) ────────────────────────────
    async function saveWifi() {
      const ssid = document.getElementById('ssid-input').value.trim();
      const pass = document.getElementById('pass-input').value;
      const msg  = document.getElementById('setup-msg');
      const btn  = document.getElementById('connect-btn');

      if (!ssid) {
        msg.className = 'msg err';
        msg.textContent = 'Please enter a network name.';
        return;
      }

      btn.textContent = 'Connecting...';
      btn.disabled = true;
      msg.className = 'msg';

      try {
        await fetch('/save', {
          method: 'POST',
          body: new URLSearchParams({ ssid: ssid, pass: pass })
        });
        msg.className = 'msg ok';
        msg.textContent = 'Credentials saved — device is rebooting. Reconnect to AquaLux to continue.';
      } catch(_) {
        msg.className = 'msg ok';
        msg.textContent = 'Device is rebooting. Reconnect to AquaLux to continue.';
      }
    }

    // ── NTP sync ──────────────────────────────────────────────
    async function syncTime() {
      const btn = document.getElementById('sync-btn');
      btn.textContent = 'Syncing...';
      btn.disabled = true;
      try {
        const r = await fetch('/synctime', { method: 'POST' });
        const ok = (await r.text()) === 'OK';
        btn.textContent = ok ? 'Synced!' : 'Failed';
      } catch(_) {
        btn.textContent = 'Failed';
      }
      setTimeout(() => { btn.textContent = 'Sync Time Now'; btn.disabled = false; }, 2500);
    }

    // ── WiFi scan ─────────────────────────────────────────────
    async function scanWifi() {
      const btn  = document.getElementById('scan-btn');
      const list = document.getElementById('scan-list');
      btn.textContent = 'Scanning...';
      btn.disabled = true;
      list.classList.remove('visible');
      list.innerHTML = '';
      try {
        const r       = await fetch('/scan');
        const networks = await r.json();
        if (networks.length === 0) {
          list.innerHTML = '<div class="scan-item"><span class="ssid-name" style="color:#555;">No networks found</span></div>';
        } else {
          networks.sort((a, b) => b.rssi - a.rssi);
          list.innerHTML = networks.map(n => {
            const bars = n.rssi > -60 ? 'Strong' : n.rssi > -75 ? 'Good' : 'Weak';
            return `<div class="scan-item" onclick="selectNetwork('${n.ssid.replace(/'/g,"\\'")}')">
              <span class="ssid-name">${n.ssid}</span>
              <span class="rssi">${bars} (${n.rssi} dBm)</span>
            </div>`;
          }).join('');
        }
        list.classList.add('visible');
      } catch(_) {
        list.innerHTML = '<div class="scan-item"><span class="ssid-name" style="color:#f87171;">Scan failed</span></div>';
        list.classList.add('visible');
      }
      btn.textContent = 'Scan';
      btn.disabled = false;
    }

    function selectNetwork(ssid) {
      document.getElementById('ssid-input').value = ssid;
      document.getElementById('scan-list').classList.remove('visible');
      document.getElementById('pass-input').focus();
    }

    // ── Buzzer toggle (dev mode) ──────────────────────────────
    async function setBuzzer(on) {
      try {
        await fetch('/buzzer', {
          method: 'POST',
          body: new URLSearchParams({ on: on ? '1' : '0' })
        });
      } catch(_) {}
    }

    // ── Status polling ────────────────────────────────────────
    function badge(id, on) {
      const el = document.getElementById(id);
      if (!el) return;
      el.textContent = on ? 'ON' : 'OFF';
      el.className   = 'badge ' + (on ? 'on' : 'off');
    }

    async function poll() {
      try {
        const r    = await fetch('/status');
        const d    = await r.json();
        const ring = !!d.alarmActive;

        // ── Alarm panel ───────────────────────────────────────
        const dot = document.getElementById('status-dot');
        dot.className = 'status-dot' + (ring ? ' alarming' : '');
        document.getElementById('status-label').textContent =
          ring ? 'Alarm is ringing!' : 'Not ringing';

        const inp = document.getElementById('alarm-input');
        const btn = document.getElementById('save-btn');
        inp.disabled = ring;
        btn.disabled = ring;

        // Only push server value when user is not actively editing the field
        if (d.alarmTime && document.activeElement !== inp) {
          inp.value = d.alarmTime;
        }

        // ── Setup panel time display ──────────────────────────
        document.getElementById('setup-time').textContent = d.time || '--:--:--';

        // ── Dev panel ─────────────────────────────────────────
        document.getElementById('cur-time').textContent = d.time || '--:--:--';
        const capRawEl = document.getElementById('cap-raw');
        if (capRawEl) capRawEl.textContent = d.capRaw ?? '--';
        badge('b-cap',   d.capDetected);
        badge('b-light', d.lightDetected);
        badge('b-alarm', d.alarmActive);
        document.getElementById('toggle-buzzer').checked = !!d.buzzerOn;

      } catch(_) {}
    }

    poll();
    setInterval(poll, 500);
  </script>

</body>
</html>
)rawliteral";

// ── setupWebServer ────────────────────────────────────────────
void setupWebServer() {
    if (serverStarted) return;

    // ── GET / ─────────────────────────────────────────────────
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", INDEX_HTML);
    });

    // ── GET /status ───────────────────────────────────────────
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String t = timeClient.isTimeSet() ? timeClient.getFormattedTime() : "--:--:--";
        String j = "{";
        j += "\"time\":"          "\"" + t             + "\",";
        j += "\"alarmTime\":"     "\"" + alarmTimeStr   + "\",";
        j += "\"buzzerOn\":"          + String(buzzerTestState ? "true" : "false") + ",";
        j += "\"alarmActive\":"       + String(alarmActive     ? "true" : "false") + ",";
        j += "\"capRaw\":"             + String(capRawValue) + ",";
        j += "\"capDetected\":"       + String(capDetected     ? "true" : "false") + ",";
        j += "\"lightDetected\":"     + String(lightDetected   ? "true" : "false");
        j += "}";
        request->send(200, "application/json", j);
    });

    // ── POST /synctime ────────────────────────────────────────
    // Triggers an NTP sync using stored credentials. Returns "OK" or "FAIL".
    server.on("/synctime", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool ok = syncNTP();
        request->send(200, "text/plain", ok ? "OK" : "FAIL");
    });

    // ── GET /scan ─────────────────────────────────────────────
    // Synchronous WiFi scan — takes ~2 s. Returns JSON array of {ssid, rssi}.
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        int n = WiFi.scanNetworks();
        String j = "[";
        for (int i = 0; i < n; i++) {
            if (i > 0) j += ",";
            String ssid = WiFi.SSID(i);
            ssid.replace("\\", "\\\\");
            ssid.replace("\"", "\\\"");
            j += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        j += "]";
        WiFi.scanDelete();
        request->send(200, "application/json", j);
    });

    // ── POST /setalarm ────────────────────────────────────────
    // Updates the alarm time in NVS and globals without rebooting.
    server.on("/setalarm", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("alarm", true)) {
            request->send(400, "text/plain", "Missing alarm param");
            return;
        }
        String newAlarm = request->getParam("alarm", true)->value();
        alarmTimeStr = newAlarm;
        parseAlarmTime(newAlarm);           // Updates alarmHour / alarmMinute globals
        preferences.begin(PREF_NAMESPACE, false);
        preferences.putString(PREF_KEY_ALARM, newAlarm);
        preferences.end();
        Serial.printf("[ALARM] Updated to '%s'\n", newAlarm.c_str());
        request->send(200, "text/plain", "OK");
    });

    // ── POST /buzzer ──────────────────────────────────────────
    server.on("/buzzer", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool on = request->hasParam("on", true) &&
                  request->getParam("on", true)->value() == "1";
        buzzerTestState = on;
        digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
        Serial.printf("[DEV] Buzzer manually set %s\n", on ? "ON" : "OFF");
        request->send(200, "text/plain", "OK");
    });

    // ── POST /save ────────────────────────────────────────────
    // Saves WiFi credentials (and optionally alarm time) then reboots.
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        String newSSID  = request->hasParam("ssid",  true)
                          ? request->getParam("ssid",  true)->value() : storedSSID;
        String newPass  = request->hasParam("pass",  true)
                          ? request->getParam("pass",  true)->value() : storedPassword;
        String newAlarm = request->hasParam("alarm", true)
                          ? request->getParam("alarm", true)->value() : alarmTimeStr;

        preferences.begin(PREF_NAMESPACE, false);
        preferences.putString(PREF_KEY_SSID,  newSSID);
        preferences.putString(PREF_KEY_PASS,  newPass);
        preferences.putString(PREF_KEY_ALARM, newAlarm);
        preferences.end();

        Serial.printf("[SAVE] SSID='%s'  Alarm='%s' — rebooting\n",
                      newSSID.c_str(), newAlarm.c_str());
        request->send(200, "text/plain", "OK");
        delay(300);
        ESP.restart();
    });

    server.begin();
    serverStarted = true;
    Serial.println("[SERVER] Web server running at " AP_GATEWAY_IP);
}
