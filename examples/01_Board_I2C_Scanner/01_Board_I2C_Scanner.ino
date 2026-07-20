/*
 * 01_Board_I2C_Scanner — SensairShuttle library
 *
 * Smallest possible starting point for the ESP-SensairShuttle:
 * powers the board up, scans the I2C bus and prints which devices
 * answered, and reports BOOT button presses.
 *
 * Expected devices:
 *   0x15 — CST816S touch panel (on the display flex cable)
 *   0x50 — BS8112 touch-button controller
 *   0x76 — BME690 (only with the environmental shuttle board inserted)
 *   0x68 — BMI270 (only with the motion shuttle board inserted)
 * The BMM350 will NOT show up here — it sits behind the BMI270's
 * auxiliary interface (see example 06).
 *
 * Board setup (Arduino IDE): Tools > Board > "ESP32C5 Dev Module"
 * (requires the arduino-esp32 core 3.3.0 or newer).
 */
#include <SensairBoard.h>

SensairBoard board;

void setup() {
  Serial.begin(115200);
  delay(1000);  /* give USB-CDC a moment */

  Serial.println();
  Serial.println("=== ESP-SensairShuttle board check ===");

  if (!board.begin()) {
    Serial.println("I2C init failed!");
    while (true) delay(1000);
  }

  board.i2cScan(Serial);
  Serial.println();
  Serial.println("Press the BOOT button to re-scan.");
}

void loop() {
  static bool wasPressed = false;
  bool pressed = board.buttonPressed();

  if (pressed && !wasPressed) {
    Serial.println();
    Serial.println("BOOT button pressed — scanning again:");
    board.i2cScan(Serial);
  }
  wasPressed = pressed;
  delay(20);
}
