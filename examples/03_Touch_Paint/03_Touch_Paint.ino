/*
 * 03_Touch_Paint — SensairShuttle library
 *
 * Minimal use of the CST816S touch panel: draw with your finger.
 * Consecutive samples are connected with lines so fast strokes stay
 * solid. Tap the CLR box in the top-right corner to erase (the chip's
 * double-tap gesture also clears, when it arrives — gesture delivery
 * is best-effort because the touch interrupt line is not wired on
 * this board and the panel is polled).
 *
 * While drawing, raw and mapped coordinates are printed to Serial —
 * handy to verify the orientation mapping on your unit.
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 */
#include <SensairDisplay.h>
#include <SensairTouch.h>

SensairDisplay display;
SensairTouch touch;

const int16_t M = SENSAIR_LCD_MARGIN;   /* keep clear of the rounded corners */
const int16_t CLR_W = 46, CLR_H = 24;
int16_t clrX, clrY;                     /* clear-button position */

void drawUi() {
  clrX = display.width() - M - CLR_W;
  clrY = M;
  display.fillScreen(SENSAIR_BLACK);
  display.setTextSize(1);
  display.setCursor(M, M + 6);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.println("draw with a finger - tap CLR to erase");
  /* clear "button", inset from the rounded top-right corner */
  display.fillRect(clrX, clrY, CLR_W, CLR_H, SENSAIR_DARKGREY);
  display.drawRect(clrX, clrY, CLR_W, CLR_H, SENSAIR_GREY);
  display.setCursor(clrX + 6, clrY + 5);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
  display.print("CLR");
}

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setRotation(1);

  if (!touch.begin()) {
    Serial.println("CST816S touch controller not found!");
    display.setCursor(M, 40);
    display.setTextColor(SENSAIR_RED);
    display.println("touch not found");
    while (true) delay(1000);
  }
  touch.setRotation(display.rotation());  /* keep touch and display aligned */

  Serial.printf("Touch OK, chip ID 0x%02X\n", touch.chipId());
  drawUi();
}

void loop() {
  static bool drawing = false;
  static int16_t lastX = 0, lastY = 0;
  static uint32_t lastLog = 0;

  SensairTouchPoint p;
  bool got = touch.read(p);

  if (got && p.touched) {
    /* CLR box hit? (only on first contact of a stroke) */
    if (!drawing && p.x >= clrX && p.x < clrX + CLR_W &&
        p.y >= clrY && p.y < clrY + CLR_H) {
      drawUi();
      delay(200);   /* debounce the button tap */
      return;
    }

    /* connect consecutive samples so fast strokes stay solid */
    if (drawing) {
      display.drawLine(lastX, lastY, p.x, p.y, SENSAIR_CYAN);
    }
    display.fillCircle(p.x, p.y, 2, SENSAIR_CYAN);
    lastX = p.x;
    lastY = p.y;
    drawing = true;

    if (millis() - lastLog > 250) {
      lastLog = millis();
      Serial.printf("raw (%3d, %3d)  ->  screen (%3d, %3d)\n",
                    p.rawX, p.rawY, p.x, p.y);
    }
  } else {
    drawing = false;   /* finger lifted: next contact starts a new stroke */
  }

  if (got && p.gesture != SENSAIR_GESTURE_NONE) {
    Serial.printf("gesture: %s\n", SensairTouch::gestureName(p.gesture));
    if (p.gesture == SENSAIR_GESTURE_DOUBLE_TAP) {
      drawUi();
    }
  }

  delay(5);   /* ~200 Hz polling */
}
