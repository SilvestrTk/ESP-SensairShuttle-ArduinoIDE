# SensairShuttle — Arduino library for the ESP-SensairShuttle

Arduino library for the **Espressif ESP-SensairShuttle** development board
(ESP32-C5 + Bosch Sensortec), with **one module per file** and **one example
per module**.

| Module | File(s) | What it drives |
|---|---|---|
| `SensairBoard`   | `src/SensairBoard.*`   | Peripheral power rail, shared I2C bus, BOOT button, labeled I2C scanner |
| `SensairDisplay` | `src/SensairDisplay.*` | 1.83" 240×284 LCD (ST7789P3, SPI) — self-contained graphics + text, no extra library needed |
| `SensairTouch`   | `src/SensairTouch.*`   | CST816S capacitive touch panel (polled, with gestures) |
| `SensairBME690`  | `src/SensairBME690.*`  | ShuttleBoard-BME690: temperature, humidity, pressure, gas resistance |
| `SensairBMI270`  | `src/SensairBMI270.*`  | ShuttleBoard-BMI270&BMM350: 6-axis IMU (accel + gyro) |
| `SensairBMM350`  | `src/SensairBMM350.*`  | Same shuttle board: 3-axis magnetometer — direct on the bus (0x14) or through the BMI270 aux interface (see note below) |
| `SensairMic`     | `src/SensairMic.*`     | Analog microphone (op-amp into ADC): sound level meter / sound detection |
| `SensairSpeaker` | `src/SensairSpeaker.*` | Speaker via differential PDM + NS4150B amp: tones, beeps, raw 16-bit PCM |

The official **Bosch SensorAPI** drivers (BME69x v1.1.0, BMI270 v2.86.1,
BMM350 v1.10.0, BSD-3-Clause) are bundled unmodified in `src/vendor/` —
nothing else to install.

---

## Getting started

### 1. Install the ESP32 Arduino core (3.3.0 or newer)

The ESP32-C5 is supported since arduino-esp32 **3.3.0**.

1. Arduino IDE → *File → Preferences → Additional boards manager URLs*, add:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. *Tools → Board → Boards Manager* → install/update **esp32 by Espressif Systems** (≥ 3.3.0).
3. Select **Tools → Board → esp32 → ESP32C5 Dev Module**.
4. **Set *Tools → USB CDC On Boot → Enabled*** — without this there is **no
   Serial output**: the board has no USB-UART bridge chip, `Serial` only
   reaches the USB port through the ESP32-C5's native USB-CDC, and the
   board definition defaults to *Disabled*.

### 2. Install this library

Copy the whole `SensairShuttle` folder into your Arduino sketchbook's
`libraries` folder (e.g. `Documents/Arduino/libraries/SensairShuttle`),
then restart the Arduino IDE.

### 3. Run the examples

*File → Examples → SensairShuttle*:

| Example | Needs | Shows |
|---|---|---|
| `01_Board_I2C_Scanner` | just the board | power-up, I2C scan with device names, BOOT button |
| `02_Display_HelloWorld` | board | text (print/printf), colors, shapes |
| `03_Touch_Paint` | board | finger drawing, gesture names, double-tap to clear |
| `04_BME690_Environment` | BME690 shuttle board | T / RH / pressure / gas resistance on serial |
| `05_BMI270_Motion` | BMI270&BMM350 shuttle board | accel (g) + gyro (dps) on serial |
| `06_BMM350_Compass` | BMI270&BMM350 shuttle board | magnetic field + compass heading on serial |
| `07_Dashboard` | any shuttle board | auto-detects the inserted board, live values on the LCD, touch/BOOT to switch pages |
| `08_Sensor_Diagnostic` | board (+ shuttle board) | low-level I2C bus scans, chip-ID reads and full driver init with Bosch result codes — run this first when a sensor is not detected |
| `09_Touch_Buttons` | board | four on-screen buttons with press/release/cancel handling and per-button counters |
| `10_Touch_Swipe` | board | swipe detection in all four directions — software detector (reliable) side by side with the chip's gesture reports |
| `11_WiFi_Setup` | board + WiFi network | full on-device WiFi onboarding: network scan list, on-screen keyboard (3 pages) for the password, connect, then internet verification via HTTP 204 check with round-trip time |
| `12_Mic_SoundLevel` | board | sound detection with intensity: live VU meter with peak hold and threshold flag, Serial-Plotter-friendly output |
| `13_Speaker_Beep` | board + speaker | simple beeping: startup jingle, three tone buttons, volume cycling, BOOT double beep |
| `14_Rotary_Keyboard` | board | space-saving text entry: up/down arrows rotate a character (hold to auto-repeat, arrows preview neighbors), tap it to append; mode key cycles abc/ABC/123/#?!, plus backspace, cancel and OK |

Minimal sensor sketch:

```cpp
#include <SensairBME690.h>

SensairBME690 bme;

void setup() {
  Serial.begin(115200);
  bme.begin();                       // 0x76 on the shared I2C bus
}

void loop() {
  SensairEnvData env;
  if (bme.read(env)) {
    Serial.printf("%.2f C  %.1f %%  %.1f hPa  %.0f Ohm\n",
                  env.temperature, env.humidity, env.pressure, env.gasResistance);
  }
  delay(3000);
}
```

---

## Hardware reference (ESP-SensairShuttle v1.0)

MCU: **ESP32-C5-WROOM-1-N16R8** (16 MB flash, 8 MB PSRAM).
Pin map extracted from the official Espressif schematic
(`SCH-ESP-SensairShuttle-MainBoard-V1_0`); everything is available as
`SENSAIR_PIN_*` defines in `src/SensairPins.h`.

| Function | GPIO | Notes |
|---|---|---|
| I2C SDA | 2 | shared: touch, touch buttons, shuttle sensors (2.2 kΩ pull-ups) |
| I2C SCL | 3 | 100 kHz default (factory firmware value) |
| LCD MOSI | 23 | ST7789P3, 4-line SPI, 20 MHz |
| LCD SCLK | 24 | |
| LCD CS | 25 | |
| LCD DC | 26 | |
| LCD reset | — | none: tied high, panel resets via power rail |
| Peripheral power (PWR_CTRL) | 5 | **LOW = LCD+touch+audio powered** (default on) |
| Shuttle BM_CS | 10 | left floating (pull-ups keep it high → I2C mode) |
| Shuttle BM_SDO | 9 | address select: LOW → 0x76/0x68, floating → 0x77/0x69 |
| Shuttle BM_G1 (INT) | 28 | **shared with the BOOT button** |
| Shuttle BM_G2 (INT2) | 0 | |
| BOOT button | 28 | LOW when pressed |
| Speaker amp enable (NS4150B) | 1 | HIGH = amp on (`SensairSpeaker`) |
| Speaker PDM +/− | 7 / 8 | differential PDM (`SensairSpeaker`) |
| Microphone (analog, via op-amp) | 6 | ADC input (`SensairMic`) |
| WS2812 connector | 27 | external RGB strip |
| External header IO | 4 (+5 via unpopulated R14) | |

I2C addresses: CST816S touch `0x15`, BS8112 touch buttons `0x50`,
BME690 `0x76`, BMI270 `0x68`, BMM350 `0x14` *(aux bus, see below)*.

### Never drive the shuttle select lines high

BM_CS and BM_SDO reach the sensor through a **PCA9306 level shifter** to the
sensor's 1.8 V rail. Driving them **LOW** from the ESP32 is safe and passes
through cleanly; push-pull driving them **HIGH at 3.3 V** forces current
through the shifter, overdrives the 1.8 V sensor pin, and the sensor drops
off the I2C bus entirely (verified on hardware). The library therefore
drives SDO low for the default addresses, and otherwise leaves both pins as
`INPUT` — the on-board pull-ups provide the high level. Keep the same rule
in your own sketches.

### Where is the BMM350 on the I2C bus?

It depends on the shuttle board revision. The magnetometer is wired to the
**BMI270's auxiliary I2C master** (the path the Espressif factory firmware
uses), and on newer revisions (ShuttleBoard-BMI270&BMM350 **V1.1**) it is
*also* connected directly to the main bus at address **0x14**. The library
supports both; the robust pattern is direct-first with aux fallback:

```cpp
SensairBMI270 imu;
SensairBMM350 mag;
if (!mag.begin()) {      // direct on the main bus (0x14), no IMU needed
  imu.begin();           // fallback: tunnel through the BMI270
  mag.begin(imu);
}
mag.usingAux();          // which path is active
```

### Display / touch power

`PWR_CTRL` (GPIO5) gates the LCD, touch and audio rail through a P-MOSFET
and is **on by default** (10 MΩ pull-down). `SensairBoard::begin()`,
`SensairDisplay::begin()` and `SensairTouch::begin()` all drive it LOW
explicitly. The backlight is hardwired on — there is no brightness control
in this hardware revision; `display.sleep(true)` blanks the panel, or turn
off the whole rail with `board.setPeripheralPower(false)`.

---

## API overview

### SensairDisplay

```cpp
display.begin();                 // optional: begin(spiHz, spiBus)
display.setRotation(1);          // 0/2 portrait 240x284, 1/3 landscape 284x240
display.fillScreen(SENSAIR_BLACK);
display.setCursor(10, 10);
display.setTextSize(2);          // 5x7 font, scaled
display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);   // fg, bg (opaque)
display.printf("T = %.1f C\n", 21.5f);                // Print interface
display.fillRect(x, y, w, h, SENSAIR_RED);
display.drawLine(0, 0, 100, 100, SENSAIR_CYAN);
display.fillCircle(cx, cy, r, SensairDisplay::color565(255, 128, 0));
display.setAddrWindow(x, y, w, h);       // then stream w*h RGB565 pixels:
display.pushColors(buffer, w * h);
```

Rotation 1 matches the factory firmware's landscape orientation. If the
image ever appears shifted by a few pixels in a flipped rotation, trim with
`display.setPanelOffset(col, row)`.

**Rounded corners:** the display glass covers the outermost pixels with a
corner radius, so anything drawn hard into a corner is unreadable. Keep
text and interactive elements at least `SENSAIR_LCD_MARGIN` (12 px) away
from every edge — all examples follow this.

### SensairTouch

```cpp
touch.begin();
touch.setRotation(display.rotation());   // report display-space coordinates
SensairTouchPoint p;
if (touch.read(p)) {
  // p.touched, p.x, p.y, p.rawX, p.rawY, p.gesture
}
```

Gestures: `SENSAIR_GESTURE_SWIPE_UP/DOWN/LEFT/RIGHT`, `SINGLE_TAP`,
`DOUBLE_TAP`, `LONG_PRESS` (`SensairTouch::gestureName()` for printing).
Gesture delivery is **best-effort**: the touch interrupt line is not wired
on this board, so the panel is polled and a gesture report can be missed.
For anything important, detect taps in software from `p.touched`
transitions (see `07_Dashboard`). Raw coordinates (`p.rawX`/`p.rawY`) are
in the controller's portrait frame, 0..239 × 0..283; the rotation mapping
in the driver was calibrated on hardware (four-quadrant button test).

### SensairBME690

```cpp
bme.begin();                       // forced mode, heater 300 C / 100 ms
bme.setHeater(320, 150);           // optional; setHeater(0, 0) disables gas
SensairEnvData env;
bme.read(env);                     // blocks ~150 ms
// env.temperature [C], env.humidity [%], env.pressure [hPa],
// env.gasResistance [Ohm], env.gasValid, env.heaterStable
```

**What `gasResistance` means:** the BME690's gas sensor is a metal-oxide
layer on a micro-hotplate (heated to the `setHeater()` temperature for each
reading), and the value is literally that layer's electrical resistance.
VOCs (solvents, breath, cooking fumes, off-gassing) react with the hot
surface and make it more conductive: **high resistance = clean air, a
falling value = more VOCs**. It is a broadband, relative indicator — not a
concentration of any specific gas, and the absolute value drifts with
humidity and sensor age, so compare against a rolling baseline. Ignore gas
readings until `gasValid && heaterStable`. For a calibrated IAQ index /
CO₂-equivalent like the factory firmware shows, layer Bosch's proprietary
[BSEC2 library](https://github.com/boschsensortec/Bosch-BSEC2-Library) on
top.

### SensairBMI270

```cpp
imu.begin();                       // accel 100 Hz / ±4 g, gyro 200 Hz / ±2000 dps
SensairMotionData m;
imu.read(m);                       // m.ax..az [g], m.gx..gz [deg/s]
imu.dev();                         // raw bmi2_dev* for advanced Bosch features
```

### SensairBMM350

```cpp
mag.begin();                       // direct on the main bus (0x14), no IMU needed
mag.begin(imu);                    // or: aux tunnel through a started SensairBMI270
mag.usingAux();                    // which path is active
SensairMagData m;
mag.read(m);                       // m.x/y/z [uT], m.temperature [C]
SensairBMM350::heading(m);         // 0..360 deg, board held level
SensairBMM350::fieldStrength(m);   // uT (Earth: ~25..65)
mag.magneticReset();               // after contact with a strong magnet
```

All sensor modules expose `lastResult()` (Bosch API result code) and `dev()`
for direct access to the underlying Bosch SensorAPI handle. Common result
codes: `0` OK, `-3` device not found (shuttle board absent — `begin()`
probes the address first and fails fast), `-2` communication failed,
`-9` (BMI270) config load failed.

### SensairMic

```cpp
mic.begin();                          // powers the audio rail, sets up the ADC
SensairSoundLevel lv = mic.readLevel(30);   // blocking 30 ms window
// lv.rmsMv (loudness), lv.peakMv, lv.dcMv (bias), lv.samples
mic.soundDetected(40.0f);             // true when RMS >= 40 mV
```

This is a level meter (several kHz effective sampling), ideal for sound
detection, clap triggers and VU meters — not an audio recorder. The mic
bias settles for ~300 ms after power-up; ignore the first readings.

### SensairSpeaker

```cpp
speaker.begin(16000);                 // PDM output at 16 kHz sample rate
speaker.beep();                       // short 1.5 kHz beep
speaker.playTone(440, 200, 80);       // freq [Hz], duration [ms], volume 0..100
speaker.playSamples(pcm, count);      // raw mono 16-bit PCM at the begin() rate
speaker.setAmp(false);                // force the amplifier off (no idle hiss)
```

Playback is blocking. The NS4150B amplifier is enabled automatically on
first playback; tones get a 5 ms fade-in/out against clicks. The setup
(differential PDM with the inverted signal on the N pin) mirrors the
factory firmware.

---

## Troubleshooting

**No Serial output at all** → *Tools → USB CDC On Boot* must be **Enabled**
(see Getting started). The USB-C cable is fine if flashing works. After
re-flashing, re-select the COM port; the Serial Monitor baud is 115200.

**A shuttle sensor is not detected** → flash `08_Sensor_Diagnostic` and read
its output: it scans the bus in both address-select states, reads every
chip ID raw, then runs the full driver init with Bosch result codes.
Typical causes, in order of likelihood:

1. *Shuttle board seating* — the 9+7 pin connector is not keyed; with power
   off, re-seat the daughterboard firmly and check its orientation against
   the photos in the Espressif user guide. Sensors are probed once in
   `setup()`, so press reset after inserting.
2. *Board inserted while running* — power-cycle (unplug) after swapping.
3. If the diagnostic's raw chip-ID reads pass but driver init fails, report
   the printed result codes.

**Writing your own I2C code on the shared bus** — two rules learned on real
hardware:

- Never push-pull-drive BM_CS/BM_SDO (GPIO10/9) high — see the level
  shifter note in the hardware section.
- Probe an address (`beginTransmission` + `endTransmission` == 0) before
  sending data writes to a device that may be absent: on the ESP32 Arduino
  core a NACKed *data* write can leave the I2C driver in a state that makes
  the next transaction fail. All `begin()` methods in this library do this.

**Factory firmware** can always be restored from the browser (Chrome/Edge)
via [ESP Launchpad](https://esp-sensairshuttle-launchpad.pages.dev/) — handy
to rule out hardware problems. If flashing from the Arduino IDE ever fails,
hold BOOT while pressing reset to force download mode.

---

## Notes & limitations

- Audio: `SensairSpeaker` covers tones and raw PCM playback; `SensairMic`
  is a level meter. Full-bandwidth audio *recording* (continuous ADC
  sampling) and audio file playback are not implemented — the plumbing in
  both modules is a solid starting point if you need them.
- The BS8112 touch-button controller (address 0x50) is not wrapped yet.
- Gas resistance is raw (see the SensairBME690 API section for what it
  means). Bosch's IAQ index requires the proprietary BSEC2 library
  ([Bosch BSEC2 Arduino](https://github.com/boschsensortec/Bosch-BSEC2-Library)),
  which can be layered on top of `SensairBME690` if needed.
- The compass heading is not tilt-compensated; combine BMI270 + BMM350 data
  with a fusion filter for a tilt-proof compass.
- BOOT button and shuttle interrupt line G1 share GPIO28: holding BOOT at
  reset enters download mode, and sensor-interrupt use of G1 is limited.

## Sources

- [ESP-SensairShuttle user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c5/esp-sensairshuttle/user_guide_v1.0.html) (Espressif)
- [Mainboard V1.0 schematic](https://dl.espressif.com/AE/esp-dev-kits/SCH_SCH-ESP-SensairShuttle-MainBoard-V1_0_2025-12-16.pdf) — source of the pin map
- [Factory firmware](https://github.com/espressif/esp-dev-kits/tree/master/examples/esp-sensairshuttle) (esp-dev-kits) — source of the LCD init sequence and sensor configuration
- Bosch SensorAPI: [BME690](https://github.com/boschsensortec/BME690_SensorAPI), [BMI270](https://github.com/boschsensortec/BMI270_SensorAPI), [BMM350](https://github.com/boschsensortec/BMM350_SensorAPI)

## License

Library code: MIT. Bundled Bosch SensorAPI files in `src/vendor/`:
BSD-3-Clause (license text in each file header).
