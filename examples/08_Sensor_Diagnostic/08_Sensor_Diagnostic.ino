/*
 * 08_Sensor_Diagnostic — SensairShuttle library
 *
 * Low-level shuttle-board diagnostic. Run this when a sensor board is
 * inserted but not detected, and send the complete serial output for
 * analysis.
 *
 * NOTE on the select pins: BM_CS and BM_SDO pass through a PCA9306 level
 * shifter to the sensor's 1.8 V rail. They must only ever be driven LOW
 * or left floating (INPUT) — driving them HIGH at 3.3 V overdrives the
 * sensor and it drops off the bus. This sketch only uses safe states.
 *
 * Expected chip IDs: BME690 -> 0x61, BMI270 -> 0x24, BMM350 -> 0x33,
 * CST816S touch -> 0xB4/0xB5/0xB6.
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 */
#include <Wire.h>
#include <SensairPins.h>
#include <SensairBME690.h>
#include <SensairBMI270.h>
#include <SensairBMM350.h>

SensairBME690 bme;
SensairBMI270 imu;
SensairBMM350 mag;

/* Bosch result-code legend (all APIs):
 *  0 OK | -1 null ptr | -2 communication failed | -3 device not found
 *  BMI270 specific: -5..-7 invalid accel/gyro config | -9 config load failed
 */
const char *rcText(int8_t rc) {
  switch (rc) {
    case 0:  return "OK";
    case -1: return "null pointer";
    case -2: return "communication failed";
    case -3: return "device not found";
    case -9: return "config load failed";
    default: return "see vendor *_defs.h";
  }
}

void scanBus(const char *label) {
  Serial.printf("scan (%s): ", label);
  uint8_t found = 0;
  for (uint8_t a = 0x08; a <= 0x77; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("0x%02X ", a);
      found++;
    }
  }
  Serial.println(found ? "" : "-- nothing --");
}

/* Read one register; `skip` = dummy bytes to discard first (BMM350 I2C
 * reads return 2 dummy bytes before the register data). */
void readReg(const char *what, uint8_t addr, uint8_t reg, uint8_t expected, uint8_t skip = 0) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  uint8_t wrc = Wire.endTransmission(false);   /* repeated start */
  if (wrc != 0) {
    Serial.printf("%-28s addr 0x%02X reg 0x%02X : write failed, Wire rc=%u\n",
                  what, addr, reg, wrc);
    Wire.beginTransmission(addr);
    Wire.endTransmission();   /* release the bus */
    return;
  }
  uint8_t want = skip + 1;
  uint8_t n = Wire.requestFrom(addr, want);
  if (n != want) {
    Serial.printf("%-28s addr 0x%02X reg 0x%02X : read returned %u bytes\n",
                  what, addr, reg, n);
    return;
  }
  uint8_t v = 0;
  for (uint8_t i = 0; i < want; i++) v = Wire.read();
  Serial.printf("%-28s addr 0x%02X reg 0x%02X : 0x%02X %s\n", what, addr, reg, v,
                v == expected ? "(expected - OK!)" : "");
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("=== ESP-SensairShuttle sensor diagnostic ===");
  Serial.printf("core: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR,
                ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

  /* Peripheral rail on (touch power), I2C up. */
  pinMode(SENSAIR_PIN_PWR_CTRL, OUTPUT);
  digitalWrite(SENSAIR_PIN_PWR_CTRL, LOW);
  delay(100);
  Wire.begin(SENSAIR_PIN_I2C_SDA, SENSAIR_PIN_I2C_SCL, SENSAIR_I2C_FREQ_DEFAULT);

  /* ---- pass 1: select pins floating -> BME690 0x77 / BMI270 0x69 ----- */
  pinMode(SENSAIR_PIN_SHUTTLE_CS, INPUT);
  pinMode(SENSAIR_PIN_SHUTTLE_SDO, INPUT);
  delay(10);
  scanBus("CS float, SDO float");

  /* ---- pass 2: SDO driven low -> BME690 0x76 / BMI270 0x68 ----------- */
  pinMode(SENSAIR_PIN_SHUTTLE_SDO, OUTPUT);
  digitalWrite(SENSAIR_PIN_SHUTTLE_SDO, LOW);
  delay(10);
  scanBus("CS float, SDO low ");

  /* ---- raw register reads (SDO still low) ---------------------------- */
  Serial.println();
  readReg("touch CST816S chip ID", SENSAIR_ADDR_TOUCH, 0xA7, 0xB4);
  readReg("BME690 chip ID (SDO low)", 0x76, 0xD0, 0x61);
  readReg("BMI270 chip ID (SDO low)", 0x68, 0x00, 0x24);
  readReg("BMM350 chip ID (main bus)", 0x14, 0x00, 0x33, 2 /* dummy bytes */);

  /* ---- and with SDO floating ----------------------------------------- */
  pinMode(SENSAIR_PIN_SHUTTLE_SDO, INPUT);
  delay(10);
  readReg("BME690 chip ID (SDO float)", 0x77, 0xD0, 0x61);
  readReg("BMI270 chip ID (SDO float)", 0x69, 0x00, 0x24);

  /* ---- full driver init, with Bosch result codes ---------------------- */
  Serial.println();
  Serial.println("full driver init attempts:");

  bool bmeOk = bme.begin();
  Serial.printf("  BME690 begin():        %-4s (rc %d: %s)\n",
                bmeOk ? "OK" : "FAIL", bme.lastResult(), rcText(bme.lastResult()));

  bool imuOk = imu.begin();
  Serial.printf("  BMI270 begin():        %-4s (rc %d: %s)\n",
                imuOk ? "OK" : "FAIL", imu.lastResult(), rcText(imu.lastResult()));

  bool magOk = mag.begin();
  Serial.printf("  BMM350 begin() direct: %-4s (rc %d: %s)\n",
                magOk ? "OK" : "FAIL", mag.lastResult(), rcText(mag.lastResult()));
  if (!magOk && imuOk) {
    magOk = mag.begin(imu);
    Serial.printf("  BMM350 begin(imu) aux: %-4s (rc %d: %s)\n",
                  magOk ? "OK" : "FAIL", mag.lastResult(), rcText(mag.lastResult()));
  }

  /* one sample from whatever initialized */
  if (bmeOk) {
    SensairEnvData e;
    if (bme.read(e)) {
      Serial.printf("  sample: %.2f C  %.1f %%RH  %.1f hPa  %.0f Ohm\n",
                    e.temperature, e.humidity, e.pressure, e.gasResistance);
    }
  }
  if (imuOk) {
    SensairMotionData m;
    if (imu.read(m)) {
      Serial.printf("  sample: acc %+ .2f %+ .2f %+ .2f g   gyro %+ .1f %+ .1f %+ .1f dps\n",
                    m.ax, m.ay, m.az, m.gx, m.gy, m.gz);
    }
  }
  if (magOk) {
    SensairMagData m;
    if (mag.read(m)) {
      Serial.printf("  sample: mag %+ .1f %+ .1f %+ .1f uT (heading %.0f deg, %s mode)\n",
                    m.x, m.y, m.z, SensairBMM350::heading(m),
                    mag.usingAux() ? "aux" : "direct");
    }
  }

  Serial.println();
  Serial.println("done — copy everything above when reporting.");
}

void loop() {
  delay(1000);
}
