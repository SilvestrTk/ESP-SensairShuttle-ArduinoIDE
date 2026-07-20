/*
 * 04_BME690_Environment — SensairShuttle library
 *
 * Reads the BME690 environmental shuttle board every 3 seconds:
 * temperature, relative humidity, barometric pressure and gas
 * resistance (a raw air-quality indicator: higher resistance
 * generally means cleaner air; let it burn in for a few minutes).
 *
 * Requires the ShuttleBoard-BME690 to be inserted in the connector
 * on the back of the ESP-SensairShuttle.
 *
 * Board setup (Arduino IDE): Tools > Board > "ESP32C5 Dev Module"
 */
#include <SensairBME690.h>

SensairBME690 bme;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== BME690 environmental sensor ===");

  if (!bme.begin()) {
    Serial.printf("BME690 not found (Bosch result %d).\n", bme.lastResult());
    Serial.println("Is the BME690 shuttle board inserted?");
    while (true) delay(1000);
  }
  Serial.println("BME690 initialized (T x2 / P x16 / RH x1, heater 300 C / 100 ms).");
  Serial.println();
}

void loop() {
  SensairEnvData env;
  if (bme.read(env)) {
    Serial.printf("T = %6.2f C   RH = %5.1f %%   p = %7.2f hPa   gas = %8.0f Ohm %s\n",
                  env.temperature,
                  env.humidity,
                  env.pressure,
                  env.gasResistance,
                  env.gasValid && env.heaterStable ? "" : "(gas warming up)");
  } else {
    Serial.printf("read failed (Bosch result %d)\n", bme.lastResult());
  }
  delay(3000);
}
