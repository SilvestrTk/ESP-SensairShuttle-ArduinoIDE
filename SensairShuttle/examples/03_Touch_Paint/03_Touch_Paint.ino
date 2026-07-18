/*
 * 03_Touch_Paint — SensairShuttle library
 *
 * Minimal use of the CST816S touch panel: draw with your finger,
 * watch gestures on the serial monitor, double-tap to clear.
 *
 * The touch controller has no interrupt line wired on this board,
 * so it is simply polled in loop().
 *
 * Board setup (Arduino IDE): Tools > Board > "ESP32C5 Dev Module"
 */
#include <SensairDisplay.h>
#include <SensairTouch.h>

SensairDisplay display;
SensairTouch touch;

void drawUi() {
  display.fillScreen(SENSAIR_BLACK);
  display.setCursor(5, 5);
  display.setTextSize(1);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.println("draw with a finger, double-tap to clear");
}

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setRotation(1);

  if (!touch.begin()) {
    Serial.println("CST816S touch controller not found!");
    display.setCursor(10, 40);
    display.setTextColor(SENSAIR_RED);
    display.println("touch not found");
    while (true) delay(1000);
  }
  touch.setRotation(display.rotation());  /* keep touch and display aligned */

  Serial.printf("Touch OK, chip ID 0x%02X\n", touch.chipId());
  drawUi();
}

void loop() {
  SensairTouchPoint p;
  if (touch.read(p)) {
    if (p.touched) {
      display.fillCircle(p.x, p.y, 3, SENSAIR_CYAN);
    }
    if (p.gesture != SENSAIR_GESTURE_NONE) {
      Serial.printf("gesture: %s at (%d, %d)\n",
                    SensairTouch::gestureName(p.gesture), p.x, p.y);
      if (p.gesture == SENSAIR_GESTURE_DOUBLE_TAP) {
        drawUi();
      }
    }
  }
  delay(10);  /* ~100 Hz polling */
}
