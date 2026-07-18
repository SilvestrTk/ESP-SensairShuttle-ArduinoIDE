# ESP-SensairShuttle Custom

Custom Arduino support for the **Espressif ESP-SensairShuttle** development
kit — an ESP32-C5 board made with Bosch Sensortec, featuring a 1.83" touch
display and swappable Bosch sensor daughterboards.

## What's here

| Folder | Contents |
|---|---|
| [`SensairShuttle/`](SensairShuttle/) | Arduino library: one driver module per file, one example per module, official Bosch SensorAPI drivers bundled. See its [README](SensairShuttle/README.md) for full setup, pinout, API reference and troubleshooting. |

## Hardware

- **MCU:** ESP32-C5-WROOM-1-N16R8 (Wi-Fi 6, BLE, 16 MB flash, 8 MB PSRAM)
- **Display:** 1.83" 240×284 LCD (ST7789P3, SPI) with CST816S capacitive touch
- **Sensor boards** (swappable, on the rear shuttle connector):
  - *ShuttleBoard-BME690* — temperature, humidity, pressure, gas/VOC
  - *ShuttleBoard-BMI270&BMM350* — 6-axis IMU + 3-axis magnetometer

## Quick start

1. Arduino IDE with **arduino-esp32 core ≥ 3.3.0**, board **ESP32C5 Dev Module**,
   and **USB CDC On Boot: Enabled** (required for Serial output).
2. Copy `SensairShuttle/` into `Documents/Arduino/libraries/`.
3. Open *File → Examples → SensairShuttle → 07_Dashboard* and upload — it
   auto-detects the inserted sensor board and shows live readings on the LCD.

If a sensor isn't detected, run example `08_Sensor_Diagnostic` and check the
[troubleshooting guide](SensairShuttle/README.md#troubleshooting).

## Status

All 13 examples compile (arduino-esp32 3.3.10); display, touch, BME690,
BMI270 and BMM350 (direct + aux access) are verified working on hardware.
Also covered: WiFi onboarding with on-screen keyboard, microphone level
meter, and speaker beeps (differential PDM). Not covered yet: BS8112 touch
buttons, audio recording/file playback.

## References

- [Official user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c5/esp-sensairshuttle/index.html) (Espressif esp-dev-kits)
- [Factory firmware web flasher](https://esp-sensairshuttle-launchpad.pages.dev/) — restore the original demo anytime

## License

Library code MIT; bundled Bosch SensorAPI drivers BSD-3-Clause
(see [SensairShuttle/LICENSE](SensairShuttle/LICENSE)).
