/*
 * 10_Touch_Swipe — SensairShuttle library
 *
 * Swipe detection in all four directions (left/right/up/down), for
 * debugging touch behavior. Two independent detectors run side by side:
 *
 *   - SOFTWARE: tracks the finger from touch-down to release and
 *     classifies the movement (>= 40 px along the dominant axis).
 *     This drives the big arrow on screen — it is the reliable one.
 *   - CHIP: whatever gesture the CST816's own engine reports, shown in
 *     the bottom status line and on Serial. Delivery is best-effort
 *     (no interrupt line on this board) and its up/down/left/right are
 *     in the controller's native frame, so directions may differ from
 *     the rotated screen — this example makes that visible.
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 */
#include <SensairDisplay.h>
#include <SensairTouch.h>

SensairDisplay display;
SensairTouch touch;

const int16_t M = SENSAIR_LCD_MARGIN;
const int16_t SWIPE_MIN_PX = 40;

void drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) {
  /* 3 px thick, good enough for an arrow */
  display.drawLine(x0, y0, x1, y1, c);
  display.drawLine(x0 + 1, y0, x1 + 1, y1, c);
  display.drawLine(x0, y0 + 1, x1, y1 + 1, c);
}

void drawArrow(const char *dir, uint16_t color) {
  int16_t cx = display.width() / 2;
  int16_t cy = display.height() / 2 + 8;
  int16_t L = 45;  /* half length of the shaft */

  /* clear the arrow area */
  display.fillRect(cx - L - 20, cy - L - 20, 2 * (L + 20), 2 * (L + 20), SENSAIR_BLACK);

  int16_t dx = 0, dy = 0;
  if (dir[0] == 'R') dx = 1;
  else if (dir[0] == 'L') dx = -1;
  else if (dir[0] == 'D') dy = 1;
  else dy = -1;

  int16_t tipX = cx + dx * L, tipY = cy + dy * L;
  drawThickLine(cx - dx * L, cy - dy * L, tipX, tipY, color);
  /* arrowhead: two lines angled back from the tip */
  int16_t hx = dx ? -dx : 1, hy = dy ? -dy : 1;
  if (dx) {
    drawThickLine(tipX, tipY, tipX + hx * 18, tipY - 14, color);
    drawThickLine(tipX, tipY, tipX + hx * 18, tipY + 14, color);
  } else {
    drawThickLine(tipX, tipY, tipX - 14, tipY + hy * 18, color);
    drawThickLine(tipX, tipY, tipX + 14, tipY + hy * 18, color);
  }
}

void showStatus(const char *softMsg, const char *chipMsg) {
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  display.setCursor(M, M + 14);
  display.printf("%-14s", softMsg);
  display.setTextSize(1);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.setCursor(M, display.height() - M - 8);
  display.printf("chip gesture: %-14s", chipMsg);
}

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setRotation(1);

  if (!touch.begin()) {
    Serial.println("touch controller not found!");
    while (true) delay(1000);
  }
  touch.setRotation(display.rotation());

  display.fillScreen(SENSAIR_BLACK);
  display.setTextSize(1);
  display.setCursor(M, M);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.println("swipe anywhere on the screen");
  showStatus("waiting...", "none");
}

void loop() {
  static bool touching = false;
  static int16_t startX = 0, startY = 0, lastX = 0, lastY = 0;

  SensairTouchPoint p;
  bool got = touch.read(p);
  bool down = got && p.touched;

  if (down && !touching) {          /* touch-down */
    touching = true;
    startX = lastX = p.x;
    startY = lastY = p.y;
  } else if (down) {                /* finger moving */
    lastX = p.x;
    lastY = p.y;
  } else if (!down && touching) {   /* release: classify the movement */
    touching = false;
    int16_t dx = lastX - startX;
    int16_t dy = lastY - startY;

    if (abs(dx) >= SWIPE_MIN_PX || abs(dy) >= SWIPE_MIN_PX) {
      const char *dir;
      uint16_t color;
      if (abs(dx) >= abs(dy)) {
        dir = dx > 0 ? "RIGHT" : "LEFT";
        color = dx > 0 ? SENSAIR_GREEN : SENSAIR_CYAN;
      } else {
        dir = dy > 0 ? "DOWN" : "UP";
        color = dy > 0 ? SENSAIR_ORANGE : SENSAIR_YELLOW;
      }
      char msg[20];
      snprintf(msg, sizeof(msg), "swipe %s", dir);
      drawArrow(dir, color);
      showStatus(msg, SensairTouch::gestureName(p.gesture));
      Serial.printf("software: swipe %-5s (dx %+4d, dy %+4d)\n", dir, dx, dy);
    } else {
      showStatus("tap", SensairTouch::gestureName(p.gesture));
      Serial.printf("software: tap at (%d, %d)\n", lastX, lastY);
    }
  }

  /* chip gesture reports, independently of the software detector */
  if (got && p.gesture != SENSAIR_GESTURE_NONE) {
    Serial.printf("chip:     %s\n", SensairTouch::gestureName(p.gesture));
  }

  delay(5);
}
