/*
 * 13_Speaker_Beep — SensairShuttle library
 *
 * Simple beeping through the on-board speaker (differential PDM into
 * the NS4150B amplifier). Plays a three-note startup jingle, then:
 *
 *   - tap LOW / MID / HIGH on screen for tones of rising pitch
 *   - tap VOL to cycle the volume 20 / 60 / 100 %
 *   - press the BOOT button for a double beep
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 */
#include <SensairDisplay.h>
#include <SensairTouch.h>
#include <SensairSpeaker.h>

SensairDisplay display;
SensairTouch touch;
SensairSpeaker speaker;

const int16_t M = SENSAIR_LCD_MARGIN;

struct ToneButton {
  int16_t x, y, w, h;
  const char *label;
  uint16_t freq;
  uint16_t color;
};
ToneButton btns[3] = {
  {0, 0, 0, 0, "LOW",  400,  SENSAIR_NAVY},
  {0, 0, 0, 0, "MID",  1000, SENSAIR_BLUE},
  {0, 0, 0, 0, "HIGH", 2000, SENSAIR_CYAN},
};
const uint8_t VOLUMES[3] = {20, 60, 100};
uint8_t volIdx = 1;
int16_t volX, volY, volW = 90, volH = 36;

void drawVolButton() {
  display.fillRect(volX, volY, volW, volH, SENSAIR_DARKGREY);
  display.drawRect(volX, volY, volW, volH, SENSAIR_GREY);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
  display.setCursor(volX + 8, volY + 10);
  display.printf("VOL %u%%", VOLUMES[volIdx]);
}

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setRotation(1);
  if (!touch.begin()) {
    Serial.println("touch controller not found!");
  }
  touch.setRotation(display.rotation());
  pinMode(SENSAIR_PIN_BOOT_BUTTON, INPUT_PULLUP);

  if (!speaker.begin(16000)) {
    Serial.println("speaker init failed!");
    while (true) delay(1000);
  }
  Serial.println("speaker ready");

  display.fillScreen(SENSAIR_BLACK);
  display.setTextSize(1);
  display.setCursor(M, M);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.println("tap a tone - BOOT = double beep");

  /* tone buttons */
  int16_t bw = (display.width() - 2 * M - 2 * 8) / 3;
  for (uint8_t i = 0; i < 3; i++) {
    btns[i].x = M + i * (bw + 8);
    btns[i].y = 44;
    btns[i].w = bw;
    btns[i].h = 78;
    display.fillRect(btns[i].x, btns[i].y, bw, btns[i].h, btns[i].color);
    display.drawRect(btns[i].x, btns[i].y, bw, btns[i].h, SENSAIR_GREY);
    display.setTextSize(2);
    display.setTextColor(SENSAIR_WHITE, btns[i].color);
    display.setCursor(btns[i].x + 10, btns[i].y + 18);
    display.print(btns[i].label);
    display.setTextSize(1);
    display.setCursor(btns[i].x + 10, btns[i].y + 50);
    display.printf("%u Hz", btns[i].freq);
  }
  volX = (display.width() - volW) / 2;
  volY = 150;
  drawVolButton();

  /* startup jingle */
  speaker.playTone(523, 120, VOLUMES[volIdx]);   /* C5 */
  speaker.playTone(659, 120, VOLUMES[volIdx]);   /* E5 */
  speaker.playTone(784, 180, VOLUMES[volIdx]);   /* G5 */
}

void loop() {
  /* on-screen buttons (act on touch-down) */
  static bool held = false;
  SensairTouchPoint p;
  bool down = touch.read(p) && p.touched;
  if (down && !held) {
    held = true;
    for (uint8_t i = 0; i < 3; i++) {
      if (p.x >= btns[i].x && p.x < btns[i].x + btns[i].w &&
          p.y >= btns[i].y && p.y < btns[i].y + btns[i].h) {
        Serial.printf("beep %u Hz\n", btns[i].freq);
        speaker.playTone(btns[i].freq, 150, VOLUMES[volIdx]);
      }
    }
    if (p.x >= volX && p.x < volX + volW && p.y >= volY && p.y < volY + volH) {
      volIdx = (volIdx + 1) % 3;
      drawVolButton();
      speaker.playTone(1000, 60, VOLUMES[volIdx]);
    }
  }
  if (!down) held = false;

  /* BOOT button: double beep */
  static bool wasPressed = false;
  bool pressed = digitalRead(SENSAIR_PIN_BOOT_BUTTON) == LOW;
  if (pressed && !wasPressed) {
    Serial.println("double beep");
    speaker.playTone(1500, 80, VOLUMES[volIdx]);
    delay(60);
    speaker.playTone(1500, 80, VOLUMES[volIdx]);
  }
  wasPressed = pressed;

  delay(5);
}
