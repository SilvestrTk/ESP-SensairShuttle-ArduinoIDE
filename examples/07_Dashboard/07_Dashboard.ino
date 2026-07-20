/*
 * 07_Dashboard — SensairShuttle library
 *
 * Everything together: detects which Bosch shuttle board is inserted
 * (BME690 environmental or BMI270&BMM350 motion) and shows live values
 * on the display. Tap the screen or press BOOT to toggle pages on the
 * motion board (IMU <-> compass).
 *
 * Board setup (Arduino IDE): Tools > Board > "ESP32C5 Dev Module"
 */
#include <SensairShuttle.h>

SensairBoard board;
SensairDisplay display;
SensairTouch touch;
SensairBME690 bme;
SensairBMI270 imu;
SensairBMM350 mag;

bool hasBme = false;
bool hasImu = false;
bool hasMag = false;
bool hasTouch = false;
uint8_t page = 0;   /* motion board: 0 = IMU, 1 = compass */

void header(const char *title) {
  display.fillScreen(SENSAIR_BLACK);
  display.fillRect(0, 0, display.width(), 26, SENSAIR_NAVY);
  /* extra inset: the title line sits at the height of the corner rounding */
  display.setCursor(SENSAIR_LCD_MARGIN + 8, 6);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_NAVY);
  display.print(title);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  board.begin();
  display.begin();
  display.setRotation(1);
  hasTouch = touch.begin();
  touch.setRotation(display.rotation());

  header("SensairShuttle");
  display.setCursor(SENSAIR_LCD_MARGIN,40);
  display.setTextSize(1);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.println("probing shuttle board...");

  /* Which sensor board is inserted? */
  hasBme = bme.begin();
  if (!hasBme) {
    hasImu = imu.begin();
    if (hasImu) {
      /* magnetometer: direct main-bus path first, then the aux tunnel */
      hasMag = mag.begin();
      if (!hasMag) hasMag = mag.begin(imu);
    }
  }

  Serial.printf("touch: %d  bme690: %d  bmi270: %d  bmm350: %d\n",
                hasTouch, hasBme, hasImu, hasMag);

  if (hasBme) header("Environment");
  else if (hasImu) header("Motion");
  else {
    header("No sensor");
    display.setCursor(SENSAIR_LCD_MARGIN,40);
    display.setTextSize(2);
    display.setTextColor(SENSAIR_ORANGE, SENSAIR_BLACK);
    display.println("Insert a shuttle\nboard + reset");

    /* On-screen I2C diagnostics, in case Serial is not visible:
     * expected 0x15 touch, 0x50 buttons, 0x76 BME690 or 0x68 BMI270. */
    display.setTextSize(1);
    display.setCursor(SENSAIR_LCD_MARGIN,95);
    display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
    display.println("I2C devices found:");
    display.setCursor(SENSAIR_LCD_MARGIN,105);
    display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
    uint8_t found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
      board.wire().beginTransmission(addr);
      if (board.wire().endTransmission() == 0) {
        display.printf("0x%02X ", addr);
        found++;
      }
    }
    if (!found) display.print("none - I2C bus problem?");
    display.println();
    display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
    display.printf("bme:%d imu:%d (Bosch rc %d/%d)\n",
                   hasBme, hasImu, bme.lastResult(), imu.lastResult());
  }
}

void showEnvironment() {
  SensairEnvData env;
  if (!bme.read(env)) return;

  display.setTextSize(2);
  display.setCursor(SENSAIR_LCD_MARGIN,40);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  display.printf("T: %6.1f C   \n", env.temperature);
  display.setCursor(SENSAIR_LCD_MARGIN,65);
  display.setTextColor(SENSAIR_CYAN, SENSAIR_BLACK);
  display.printf("RH: %5.1f %%   \n", env.humidity);
  display.setCursor(SENSAIR_LCD_MARGIN,90);
  display.setTextColor(SENSAIR_GREEN, SENSAIR_BLACK);
  display.printf("p: %7.1f hPa   \n", env.pressure);
  display.setCursor(SENSAIR_LCD_MARGIN,115);
  display.setTextColor(SENSAIR_YELLOW, SENSAIR_BLACK);
  if (env.gasValid && env.heaterStable) {
    display.printf("gas: %6.0f kOhm  \n", env.gasResistance / 1000.0f);
  } else {
    display.print("gas: warming up   ");
  }

  Serial.printf("T %.2f C  RH %.1f %%  p %.1f hPa  gas %.0f Ohm\n",
                env.temperature, env.humidity, env.pressure, env.gasResistance);
}

void showImu() {
  SensairMotionData m;
  if (!imu.read(m)) return;

  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  display.setCursor(SENSAIR_LCD_MARGIN,40);
  display.printf("ax %+5.2f g   \n", m.ax);
  display.setCursor(SENSAIR_LCD_MARGIN,62);
  display.printf("ay %+5.2f g   \n", m.ay);
  display.setCursor(SENSAIR_LCD_MARGIN,84);
  display.printf("az %+5.2f g   \n", m.az);
  display.setTextColor(SENSAIR_CYAN, SENSAIR_BLACK);
  display.setCursor(150, 40);
  display.printf("gx %+7.1f  \n", m.gx);
  display.setCursor(150, 62);
  display.printf("gy %+7.1f  \n", m.gy);
  display.setCursor(150, 84);
  display.printf("gz %+7.1f  \n", m.gz);

  /* bubble level: dot moves with tilt */
  int16_t cx = display.width() / 2, cy = 170;
  static int16_t px = cx, py = cy;
  display.fillCircle(px, py, 6, SENSAIR_BLACK);
  display.drawCircle(cx, cy, 45, SENSAIR_DARKGREY);
  display.drawCircle(cx, cy, 15, SENSAIR_DARKGREY);
  px = cx + (int16_t)(m.ax * 45);
  py = cy + (int16_t)(m.ay * 45);
  display.fillCircle(px, py, 6, SENSAIR_GREEN);
}

void showCompass() {
  SensairMagData m;
  if (!mag.read(m)) return;

  float hdg = SensairBMM350::heading(m);
  display.setTextSize(3);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  display.setCursor(SENSAIR_LCD_MARGIN,45);
  display.printf("%5.1f deg  ", hdg);
  display.setTextSize(1);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.setCursor(SENSAIR_LCD_MARGIN,80);
  display.printf("x %+7.1f  y %+7.1f  z %+7.1f uT   ", m.x, m.y, m.z);

  /* needle */
  int16_t cx = display.width() / 2, cy = 165;
  static float lastHdg = -1000;
  if (fabsf(hdg - lastHdg) > 1.0f) {
    display.fillCircle(cx, cy, 52, SENSAIR_BLACK);
    display.drawCircle(cx, cy, 50, SENSAIR_DARKGREY);
    float rad = (90.0f - hdg) * (float)M_PI / 180.0f;  /* needle points north */
    display.drawLine(cx, cy, cx + (int16_t)(45 * cosf(rad)),
                     cy - (int16_t)(45 * sinf(rad)), SENSAIR_RED);
    lastHdg = hdg;
  }
}

void loop() {
  /* page switching: tap or BOOT button (motion board only).
   * Taps are detected in software (touch-down -> quick release) — the
   * chip's gesture reports are best-effort without an interrupt line. */
  bool toggle = false;
  static bool touching = false;
  static uint32_t touchStart = 0;
  if (hasTouch) {
    SensairTouchPoint p;
    bool down = touch.read(p) && p.touched;
    if (down && !touching) {
      touching = true;
      touchStart = millis();
    } else if (!down && touching) {
      touching = false;
      if (millis() - touchStart < 350) toggle = true;
    }
  }
  static bool wasPressed = false;
  bool pressed = board.buttonPressed();
  if (pressed && !wasPressed) toggle = true;
  wasPressed = pressed;

  if (toggle && hasImu && hasMag) {
    page ^= 1;
    header(page == 0 ? "Motion" : "Compass");
  }

  static uint32_t lastUpdate = 0;
  uint32_t interval = hasBme ? 3000 : 100;
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    if (hasBme) showEnvironment();
    else if (hasImu && page == 0) showImu();
    else if (hasMag && page == 1) showCompass();
  }
  delay(10);
}
