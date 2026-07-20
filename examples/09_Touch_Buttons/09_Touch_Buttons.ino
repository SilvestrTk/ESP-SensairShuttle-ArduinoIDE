/*
 * 09_Touch_Buttons — SensairShuttle library
 *
 * Four clickable on-screen buttons, for testing touch accuracy and
 * responsiveness. A button highlights while your finger is on it and
 * counts the press when you release inside it (slide off to cancel —
 * standard button behavior). Presses are also printed to Serial.
 *
 * Taps are handled purely in software from p.touched transitions —
 * no reliance on the chip's gesture engine.
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 */
#include <SensairDisplay.h>
#include <SensairTouch.h>

SensairDisplay display;
SensairTouch touch;

const int16_t M = SENSAIR_LCD_MARGIN;
const int16_t GAP = 10;

struct Button {
  int16_t x, y, w, h;
  const char *label;
  uint16_t color;
  uint16_t count;
  bool held;
};

Button btns[4] = {
  {0, 0, 0, 0, "A", SENSAIR_RED,    0, false},
  {0, 0, 0, 0, "B", SENSAIR_GREEN,  0, false},
  {0, 0, 0, 0, "C", SENSAIR_BLUE,   0, false},
  {0, 0, 0, 0, "D", SENSAIR_ORANGE, 0, false},
};

void drawButton(Button &b) {
  display.fillRect(b.x, b.y, b.w, b.h, b.color);
  display.drawRect(b.x, b.y, b.w, b.h, SENSAIR_GREY);
  display.setTextSize(3);
  display.setTextColor(SENSAIR_WHITE, b.color);
  display.setCursor(b.x + 12, b.y + 10);
  display.print(b.label);
  display.setTextSize(2);
  display.setCursor(b.x + 12, b.y + b.h - 24);
  display.printf("%u", b.count);
}

/* Instant press feedback: a thin border flash costs well under a
 * millisecond, while repainting the whole button takes ~10 ms. */
void highlightButton(Button &b, bool on) {
  uint16_t c = on ? SENSAIR_WHITE : b.color;
  for (int8_t i = 1; i <= 3; i++) {
    display.drawRect(b.x + i, b.y + i, b.w - 2 * i, b.h - 2 * i, c);
  }
}

bool inside(const Button &b, int16_t px, int16_t py) {
  return px >= b.x && px < b.x + b.w && py >= b.y && py < b.y + b.h;
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
  display.println("tap the buttons");

  /* 2x2 grid, inset by the rounded-corner margin */
  int16_t top = M + 16;
  int16_t bw = (display.width() - 2 * M - GAP) / 2;
  int16_t bh = (display.height() - top - M - GAP) / 2;
  for (uint8_t i = 0; i < 4; i++) {
    btns[i].x = M + (i % 2) * (bw + GAP);
    btns[i].y = top + (i / 2) * (bh + GAP);
    btns[i].w = bw;
    btns[i].h = bh;
    drawButton(btns[i]);
  }
}

void loop() {
  /* -1 = no contact, -2 = contact outside all buttons, 0..3 = held button */
  static int8_t held = -1;

  SensairTouchPoint p;
  bool down = touch.read(p) && p.touched;

  if (down && held == -1) {
    /* new contact: which button? */
    held = -2;
    for (int8_t i = 0; i < 4; i++) {
      if (inside(btns[i], p.x, p.y)) {
        held = i;
        btns[i].held = true;
        highlightButton(btns[i], true);
        Serial.printf("press   %s at (%d, %d)\n", btns[i].label, p.x, p.y);
        break;
      }
    }
  } else if (down && held >= 0 && !inside(btns[held], p.x, p.y)) {
    /* finger slid off the button: cancel */
    btns[held].held = false;
    highlightButton(btns[held], false);
    Serial.printf("cancel  %s\n", btns[held].label);
    held = -2;
  } else if (!down && held != -1) {
    /* release */
    if (held >= 0) {
      btns[held].held = false;
      btns[held].count++;
      drawButton(btns[held]);
      Serial.printf("clicked %s (count %u)\n", btns[held].label, btns[held].count);
    }
    held = -1;
  }

  delay(5);
}
