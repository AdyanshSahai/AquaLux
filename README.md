# AquaLux

A wake-up alarm that keeps ringing until you put a water bottle on it and turn the light on.

![Platform](https://img.shields.io/badge/platform-ESP32--S2-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![Build](https://img.shields.io/badge/build-PlatformIO-orange)
![License](https://img.shields.io/badge/license-MIT-green)

I built AquaLux to make people wake up properly. Switching off a normal alarm takes one tap, and then you go back to sleep. This one only stops when you have a bottle of water on the sensor pad and the room light is on, and both have to hold for two seconds. It was a working prototype and got to the TeenTech finals. I've stopped working on it.

- **Bottle plus light to silence it.** A capacitive pad detects the bottle and a photoresistor module detects the light.
- **Set up from your phone.** The device runs its own WiFi network (`AquaLux`) with a web page for the alarm time, WiFi and sensor calibration.
- **Keeps its own time.** It syncs over NTP at boot and again at 03:00, and can be set by hand if there's no WiFi.
- **Factory reset button.** Hold it for 3 seconds to wipe all saved settings.

This is a breadboard prototype, not a finished product. See [Known Issues](#known-issues).

## Hardware

| Part | Notes |
|------|-------|
| LOLIN S2 Mini (ESP32-S2) | Uses the chip's built-in touch sensing on GPIO 5 |
| Aluminium foil pad | Kitchen foil, a circle taped to the top of the AquaLux robot and covered with electrical tape |
| Photoresistor module | Digital output (DO) pin, read on GPIO 21 |
| Buzzer | Driven through a 2N2222 transistor from GPIO 40 |
| Push button | Between GPIO 38 and ground, internal pull-up |
| 3.7 V LiPo battery | Power |
| J5109 | Also in the build |

Pin numbers and timings are all in [src/config.h](src/config.h). Do not use GPIO 33 to 37 on the S2 Mini. They are the PSRAM interface and using them as GPIO crashes the board.

There is a KiCad schematic and board files in [static/](static/). I never built a PCB. The prototype was wired on a breadboard.

## Quick Start

You need [PlatformIO](https://platformio.org/install) (the VS Code extension or the CLI).

1. Clone the repo and open it in PlatformIO.
2. Check `NTP_UTC_OFFSET_SEC` in `src/config.h`. It defaults to `3600` (UTC+1) and has to match your timezone, for example `19800` for IST or `-18000` for EST. There is no daylight saving handling.
3. Plug in the S2 Mini over USB and run:

   ```bash
   pio run -t upload
   pio device monitor
   ```

4. On your phone, join the open WiFi network `AquaLux`, then open `http://192.168.4.1`.
5. Enter your home WiFi name and password and tap **Connect & Sync**. The board reboots, joins your WiFi, fetches the time, then drops back to its own network.

At the end you should see `===== AquaLux RUNNING =====` in the serial monitor, and the web page should show the current time.

Without home WiFi you can skip step 5 and set the time by hand on the Config page. It is held in RAM, so it has to be set again after every power loss.

## Using It

1. Set the alarm time on the main page and tap **Save**. It defaults to `07:00`.
2. Calibrate the pad (see below) the first time, and again whenever you change bottle.
3. At the alarm time the buzzer pulses 500 ms on and 500 ms off.
4. Put the bottle on the pad and turn the light on. Hold both for 2 seconds and the buzzer stops.
5. If either condition drops during those 2 seconds, the timer starts again.

### Calibrating the Pad

Open the calibration section of the Config page. Put nothing on the pad and tap **Nothing**, then the bottle and tap **Bottle**, then your hand and tap **Hand**. Once all three are captured, the bottle window is set between the midpoints:

- lower bound = (nothing + bottle) / 2
- upper bound = (bottle + hand) / 2

The values are saved to flash. You can also type the bounds in directly. Without calibration it uses `17000` to `21000`, which is a starting guess from my own bottle. Readings differ from bottle to bottle, so calibrate.

## How It Works

```
  boot ──► load settings from flash
             │
             ├─ WiFi saved? ──► join home WiFi ► NTP ► drop WiFi ► own network
             └─ no WiFi?    ──► own network only (setup mode)
             │
  loop  ─────┴► reset button ► clock checks ► read sensors ► alarm pulse + dismiss check
                                  │
                                  ├─ alarm time reached ► buzzer pulses
                                  └─ 03:00 ► brief NTP resync
```

| File | What lives there |
|------|------------------|
| [src/main.cpp](src/main.cpp) | `setup()` and the main loop |
| [src/config.h](src/config.h) | Pins, timings, defaults, flash key names |
| [src/alarm.cpp](src/alarm.cpp) | Trigger, buzzer pulsing, dismiss logic |
| [src/sensors.cpp](src/sensors.cpp) | Reads the pad and the light sensor |
| [src/wifi_ntp.cpp](src/wifi_ntp.cpp) | Clock, alarm-time parsing, NTP sync |
| [src/webserver.cpp](src/webserver.cpp) | The web page (embedded HTML) and its HTTP routes |
| [src/reset_button.cpp](src/reset_button.cpp) | Hold-to-reset |
| [FLOWCHART.txt](FLOWCHART.txt) | Longer text flowchart of the whole system |
| [static/](static/) | KiCad schematic and board files |

## Configuration

Compile-time values in `src/config.h`:

| Key | Default | What it does |
|-----|---------|--------------|
| `NTP_UTC_OFFSET_SEC` | `3600` | Timezone offset in seconds |
| `NTP_SERVER` | `pool.ntp.org` | NTP server |
| `NTP_RESYNC_HOUR` / `NTP_RESYNC_MINUTE` | `3` / `0` | Daily resync time |
| `BUZZER_ON_MS` / `BUZZER_OFF_MS` | `500` / `500` | Buzzer pulse timing |
| `DISMISS_DEBOUNCE_MS` | `2000` | How long both conditions must hold |
| `RESET_HOLD_MS` | `3000` | How long to hold the reset button |
| `CAP_BOTTLE_MIN` / `CAP_BOTTLE_MAX` | `17000` / `21000` | Bottle window before calibration |
| `MAX_SAVED_NETWORKS` | `5` | Saved WiFi networks. The oldest is dropped when full |

The alarm time, WiFi networks, bottle window and WiFi connect timeout (default 15 s) are set from the web page and stored in flash.

## Design Notes

- **Dismissing needs 2 seconds of both conditions.** The capacitive readings swing a lot, and moisture in the air makes them flicker. Without the hold time the alarm would stop on a random good reading.
- **Only a window of readings counts as a bottle.** A hand reads higher than a bottle (the code comments put it at 45000 and up), so touching the pad can't silence the alarm.
- **Calibration uses midpoints between three readings.** Bottles read differently, so I wouldn't trust one fixed range. The defaults are only a starting guess.
- **The board syncs time at 03:00.** The ESP32 clock drifts a little every day and I didn't want an external clock chip. Nobody is using the alarm at 03:00.
- **Home WiFi is only joined for the sync.** Much of the project was tuned for battery life. Staying connected costs current, and phones on the setup network don't need internet.
- **WiFi transmit power is lowered to 8.5 dBm.** Same reason: less current draw.
- **No light sleep.** It switches off the WiFi radio, which drops phones connected to the setup page and stalls the web server. A plain `delay()` lets the background tasks run without touching the radio.
- **The loop polls faster during an alarm** (50 ms instead of 100 ms) so the buzzer timing stays accurate.

The code was written mostly by an AI assistant, which also did the code-level debugging. The design, the algorithm, the hardware build and the physical testing are mine.

## Security

The setup network has no password, and the web page has no login. Anyone within WiFi range can:

- change the alarm time and sensor thresholds,
- turn the buzzer on and off,
- save new WiFi credentials, which reboots the board,
- read every saved WiFi password in plain text from `/networks`.

I never fixed this. Don't put a WiFi network you care about on a device running this firmware.

## Known Issues

- The prototype is very rough. It's a breadboard with taped foil, and many of the sensor problems come from that. A PCB would fix most of them.
- Capacitive readings drift and jump. The 2-second hold hides most of it, but it isn't perfect, and humidity or a different bottle can push readings out of the calibrated window.
- The timezone is compile-time only and has no daylight saving handling. Changing it means reflashing.
- Time lives in RAM. After a power loss the clock is wrong until the board syncs or you set it by hand.
- The 03:00 resync stops the main loop while it connects, for up to the WiFi timeout (15 s by default).

## Troubleshooting

| Problem | Fix |
|---------|-----|
| No `AquaLux` network appears | Check the serial monitor for `AP up`. Reset the board. |
| Time shows `--:--:--` | Time isn't set. Use **Sync Time Now** or set it by hand on the Config page. |
| Time is wrong by an hour or more | Fix `NTP_UTC_OFFSET_SEC` in `src/config.h` and reflash. |
| Alarm never stops | Watch the pad and light values on the web page. Recalibrate the pad. Check the light sensor sees light. |
| Hand silences it, or the bottle doesn't | Recalibrate with the bottle you actually use. |
| Wrong WiFi saved | Hold the reset button for 3 seconds. You hear a short beep, then the board reboots into setup mode. |
| Board crashes when you add a button or sensor | Don't use GPIO 33 to 37 (PSRAM pins). |

## Planned Updates

I won't be doing these, since the project is dropped. If anyone picks it up, in this order:

1. Move it from the breadboard to a PCB. The schematic is already in `static/`.
2. Put a password on the setup network and remove the plain-text `/networks` route.
3. Make the timezone settable from the web page.
4. Give the pad a defined, repeatable shape instead of taped foil.

## Credits

- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) and [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) by me-no-dev
- [NTPClient](https://github.com/arduino-libraries/NTPClient) from arduino-libraries
- The Arduino core for ESP32 (WiFi, Preferences)

## License

MIT. See [LICENSE](LICENSE).
