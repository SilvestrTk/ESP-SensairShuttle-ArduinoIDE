/*
 * 05_BMI270_Motion — SensairShuttle library
 *
 * Reads the BMI270 6-axis IMU on the motion shuttle board:
 * acceleration in g and angular rate in deg/s, 10 times per second.
 *
 * Requires the ShuttleBoard-BMI270&BMM350 to be inserted in the
 * connector on the back of the ESP-SensairShuttle.
 * (For the BMM350 magnetometer on the same board, see example 06.)
 *
 * Board setup (Arduino IDE): Tools > Board > "ESP32C5 Dev Module"
 */
#include <SensairBMI270.h>

SensairBMI270 imu;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== BMI270 IMU ===");
  Serial.println("Initializing (uploads the Bosch config file, takes a moment)...");

  if (!imu.begin()) {
    Serial.printf("BMI270 not found (Bosch result %d).\n", imu.lastResult());
    Serial.println("Is the BMI270&BMM350 shuttle board inserted?");
    while (true) delay(1000);
  }
  Serial.println("BMI270 ready: accel 100 Hz / +-4 g, gyro 200 Hz / +-2000 dps.");
  Serial.println();
}

void loop() {
  SensairMotionData m;
  if (imu.read(m)) {
    Serial.printf("acc [g]  x %+6.2f  y %+6.2f  z %+6.2f  |  "
                  "gyro [dps]  x %+8.2f  y %+8.2f  z %+8.2f\n",
                  m.ax, m.ay, m.az, m.gx, m.gy, m.gz);
  } else {
    Serial.printf("read failed (Bosch result %d)\n", imu.lastResult());
  }
  delay(100);
}
