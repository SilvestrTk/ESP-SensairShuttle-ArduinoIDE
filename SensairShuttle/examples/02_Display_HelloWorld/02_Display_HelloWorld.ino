/*
 * 02_Display_HelloWorld — SensairShuttle library
 *
 * Basic use of the 1.83" LCD (ST7789P3, 240x284, SPI):
 * text with print/println/printf, colors, and simple shapes.
 *
 * The display driver is self-contained — no extra graphics library
 * needed. It inherits Print, so it works like Serial for text.
 *
 * Board setup (Arduino IDE): Tools > Board > "ESP32C5 Dev Module"
 */
#include <SensairDisplay.h>

SensairDisplay display;

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setRotation(1);           /* landscape, like the factory firmware */
  display.fillScreen(SENSAIR_BLACK);

  /* --- text ----------------------------------------------------------- */
  display.setCursor(10, 10);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  display.setTextSize(3);
  display.println("Hello World!");

  display.setCursor(10, 45);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_CYAN, SENSAIR_BLACK);
  display.println("ESP-SensairShuttle");

  display.setCursor(10, 70);
  display.setTextSize(1);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.printf("%d x %d px, rotation %d\n", display.width(), display.height(),
                 display.rotation());

  /* --- shapes --------------------------------------------------------- */
  display.drawRect(10, 90, 80, 50, SENSAIR_RED);
  display.fillRect(14, 94, 72, 42, SENSAIR_DARKGREY);
  display.drawCircle(140, 115, 25, SENSAIR_GREEN);
  display.fillCircle(200, 115, 25, SENSAIR_BLUE);
  display.drawLine(10, 160, display.width() - 10, 160, SENSAIR_YELLOW);

  /* color bar from the color565 helper */
  for (int16_t x = 0; x < display.width(); x++) {
    uint8_t v = (uint8_t)(x * 255 / display.width());
    display.drawFastVLine(x, 175, 20, SensairDisplay::color565(v, 0, 255 - v));
  }

  Serial.println("Display demo drawn.");
}

void loop() {
  /* Small live element: a counter in the bottom-left corner. */
  static uint32_t seconds = 0;
  display.setCursor(10, display.height() - 25);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_ORANGE, SENSAIR_BLACK);
  display.printf("up %lus  ", (unsigned long)seconds++);
  delay(1000);
}
