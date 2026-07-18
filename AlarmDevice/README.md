# Alarm Device — ESP-SensairShuttle

Multi-sensor alarm device: measures temperature, humidity, air quality,
vibration and sound level; when a user-defined threshold is exceeded it
raises a **local alarm** (siren over the on-board speaker) and/or a
**remote alarm** (webhook to a server, IFTTT by default). Everything is
configured on the touchscreen and persisted in NVM.

Requires the **SensairShuttle** library (in `../SensairShuttle`, copy to
`Documents/Arduino/libraries/`).

## Build (Arduino IDE)

| Tools menu setting | Value |
|---|---|
| Board | **ESP32C5 Dev Module** |
| USB CDC On Boot | **Enabled** (required for Serial) |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** — the app does not fit the default 1.2 MB scheme |

Open `AlarmDevice.ino`, compile, upload. All configuration defaults are
in **`AppConfig.h`** (user section: device ID, server URL, thresholds,
timing; compile section: debug level, watchdog, test mode).

## Using the device

**Main screen** shows the live value of every alarm source (rows show
`n/a` for sensors that are not inserted — note the board has a single
shuttle connector, so the BME690 environmental values and the BMI270
vibration value cannot be available at the same time). Right-hand
markers: `L`/`R` = local/remote enabled, red `!` = over threshold.

- **ALARMS** → list of the five alarms → tap one to edit: threshold
  (+/- with hold-repeat), *Local siren* and *Remote webhook* checkboxes,
  remote reset time in minutes, SAVE. Alarms are OFF by default; every
  change is stored in NVM immediately on SAVE.
- **CONNECT** → WiFi (scan, tap a network, enter the password with the
  rotary keyboard, credentials stored in NVM and auto-reconnected after
  power loss), Server URL, Device ID, and a one-tap reset of the URL to
  the built-in IFTTT default.
- **STOP ALARM** — while any local alarm sounds, the main screen shows
  only this button. Stopping silences all local alarms; a stopped alarm
  re-arms once its value has dropped below the threshold (no instant
  re-trigger).

Text entry uses the rotary keyboard (arrows rotate the character with
previews and hold-to-repeat, tap the character to append, mode key
cycles abc/ABC/123/#?!, backspace, X cancel, OK confirm).

## Alarm semantics

- Alarms are independent and run **in parallel**; any number can be
  triggered/overlapping at once. The speaker is owned by a single
  entity (`AlarmSiren`) which all local alarms share.
- **Local**: latches the siren until STOP ALARM is pressed.
- **Remote**: sends `GET <url>?value1=<text>&value2=<device id>` (both
  URL-encoded), e.g.
  `value1=Temperature exceeded threshold 35.0 C (measured 36.2 C)`,
  then stays quiet for the configured reset time.
- **Delivery guarantee**: WiFi is verified before sending; the alarm is
  written to NVM *before* the first attempt, retried every
  `APP_RETRY_MINUTES`, and — after a power failure or crash — reloaded
  from NVM at boot and retried until delivered (queue capacity
  `APP_PENDING_MAX`, oldest dropped when full).

## Architecture

```
AlarmDevice.ino   minimal entry point (test suite or app, per AppConfig.h)
AppConfig.h       ALL user + compile configuration
App.*             orchestration, watchdog, reset-reason logging, heap log
AlarmTypes.h      shared types + the alarm definition table declaration
AlarmManager.*    threshold evaluation (pure, testable core), payload text
AlarmSiren.*      THE single speaker owner: two-tone siren, UI chirp
Sensors.*         scheduler: BME690 env + IAQ approximation, BMI270
                  vibration (std-dev of |accel| over a window), mic RMS
AlarmNet.*        WiFi reconnect, webhook delivery, NVM-backed retry queue
Storage.*         NVM (Preferences): settings + pending queue
Ui.*              screens: main/alarm list/editor/connection/wifi scan
RotaryInput.*     rotary keyboard component (from library example 14)
DebugLog.h        [millis][level][module] Serial logging macros
Tests.*           on-device self-test suite
```

**Adding a new alarm type** = 3 small steps (see `AlarmTypes.h`): add an
id, add a row to `ALARM_DEFS[]`, return its measurement in
`Sensors::valueFor()`. Storage, evaluation, UI, siren and webhook
handling pick it up automatically.

## Air quality note

Bosch's factory IAQ requires the proprietary BSEC2 library, which cannot
be bundled. The app computes the widely used approximation (gas
resistance vs. an adaptive clean-air baseline, 75%, + humidity deviation
from 40 %RH, 25%) mapped to the factory 0–500 scale with the same level
names (Excellent…Very Bad). The gas sensor needs `APP_IAQ_WARMUP_MS`
(60 s) of burn-in after power-up before the value becomes valid, and the
baseline improves the longer the device runs in clean air.

## Robustness

- **Crash recovery**: all settings/arming/pending alarms restored from
  NVM at boot; reset reason logged; optional task watchdog
  (`APP_WATCHDOG_S`) reboots on a hung loop — blocking UI code feeds it
  via `App::backgroundTick()`.
- **Memory**: no dynamic allocation in steady state (static module
  storage; HTTP/TLS clients are stack-scoped per request). The free-heap
  watermark is logged every minute — a leak would show as a steady
  decline.
- **Debug output**: every module logs through `DebugLog.h` with
  timestamps and module tags; set `APP_DEBUG_LEVEL 4` for verbose
  per-measurement logs.

## Self-test suite

Set `APP_RUN_TESTS 1` in `AppConfig.h`, upload, open the Serial Monitor
(115200). Tests cover URL encoding, payload formatting, the full alarm
evaluation semantics (latching, ack/re-arm, remote reset period, invalid
readings, disabled alarms), the IAQ math, and the NVM layer
(settings roundtrip + pending queue FIFO/capacity) in a scratch NVS
namespace. Output is `PASS`/`FAIL` per test plus a summary line.
Set the flag back to `0` for the real application.
