/*
 * 06_BMM350_Compass — SensairShuttle library
 *
 * Reads the BMM350 3-axis magnetometer and prints the magnetic field
 * and a simple compass heading (hold the board level).
 *
 * Access path: on newer shuttle revisions (V1.1) the BMM350 also sits
 * directly on the main I2C bus (address 0x14) — tried first. On older
 * revisions it is only reachable through the BMI270's auxiliary I2C
 * interface — used as fallback (needs the BMI270 initialized first).
 *
 * Requires the ShuttleBoard-BMI270&BMM350 to be inserted.
 *
 * Board setup (Arduino IDE): Tools > Board > "ESP32C5 Dev Module"
 */
#include <SensairBMI270.h>
#include <SensairBMM350.h>

SensairBMI270 imu;
SensairBMM350 mag;

const char *cardinal(float deg) {
  if (deg >= 315 || deg < 45)  return "N";
  if (deg < 135)               return "E";
  if (deg < 225)               return "S";
  return "W";
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== BMM350 compass ===");

  /* Try the direct main-bus path first, fall back to the aux tunnel. */
  if (!mag.begin()) {
    Serial.println("BMM350 not on the main bus, trying via BMI270 aux...");
    if (!imu.begin()) {
      Serial.printf("BMI270 not found (Bosch result %d) — it is required "
                    "for the aux path.\n", imu.lastResult());
      while (true) delay(1000);
    }
    if (!mag.begin(imu)) {
      Serial.printf("BMM350 init failed (Bosch result %d).\n", mag.lastResult());
      while (true) delay(1000);
    }
  }
  Serial.printf("BMM350 ready (%s mode): 25 Hz, all axes.\n",
                mag.usingAux() ? "aux tunnel" : "direct");
  Serial.println();
}

void loop() {
  SensairMagData m;
  if (mag.read(m)) {
    float hdg = SensairBMM350::heading(m);
    float field = SensairBMM350::fieldStrength(m);
    Serial.printf("mag [uT]  x %+7.2f  y %+7.2f  z %+7.2f  |  "
                  "field %6.2f uT  |  heading %5.1f deg (%s)\n",
                  m.x, m.y, m.z, field, hdg, cardinal(hdg));
    if (field > 120.0f) {
      Serial.println("  field unusually strong — magnet nearby? "
                     "Consider mag.magneticReset().");
    }
  } else {
    Serial.printf("read failed (Bosch result %d)\n", mag.lastResult());
  }
  delay(200);
}
