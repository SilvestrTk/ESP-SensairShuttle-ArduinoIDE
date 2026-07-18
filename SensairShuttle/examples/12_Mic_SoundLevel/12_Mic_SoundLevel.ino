/*
 * 12_Mic_SoundLevel — SensairShuttle library
 *
 * Sound detection with intensity: a live VU meter on the display.
 * Shows the microphone's RMS level as a bar with a peak-hold marker,
 * flashes "SOUND!" when the level crosses the threshold, and streams
 * the numbers to Serial (plot-friendly with the Serial Plotter).
 *
 * Try clapping, speaking or playing music near the microphone.
 * Note: the mic bias settles for a moment after power-up, so the first
 * readings are ignored.
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 */
#include <SensairDisplay.h>
#include <SensairMic.h>

SensairDisplay display;
SensairMic mic;

const int16_t M = SENSAIR_LCD_MARGIN;
const float THRESHOLD_MV = 40.0f;   /* "sound detected" RMS level */
const float FULLSCALE_MV = 300.0f;  /* bar full scale */

int16_t barX, barY, barW, barH;
float peakHold = 0;
uint32_t peakHoldUntil = 0;

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setRotation(1);
  mic.begin();

  display.fillScreen(SENSAIR_BLACK);
  display.setTextSize(1);
  display.setCursor(M, M);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.println("microphone level meter");

  barX = M;
  barY = 120;
  barW = display.width() - 2 * M;
  barH = 34;
  display.drawRect(barX - 1, barY - 1, barW + 2, barH + 2, SENSAIR_GREY);

  /* threshold marker below the bar */
  int16_t tx = barX + (int16_t)(barW * THRESHOLD_MV / FULLSCALE_MV);
  display.drawFastVLine(tx, barY + barH + 4, 8, SENSAIR_ORANGE);
  display.setCursor(M, barY + barH + 16);
  display.printf("threshold %.0f mV, full scale %.0f mV", THRESHOLD_MV, FULLSCALE_MV);

  /* skip the bias settle time */
  delay(300);
}

void loop() {
  SensairSoundLevel lv = mic.readLevel(30);

  /* --- numbers -------------------------------------------------------- */
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  display.setCursor(M, 40);
  display.printf("RMS  %5.1f mV  ", lv.rmsMv);
  display.setCursor(M, 62);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.printf("peak %5.1f mV  ", lv.peakMv);

  /* --- detection flag ------------------------------------------------- */
  display.setTextSize(3);
  display.setCursor(display.width() - M - 110, 44);
  if (lv.rmsMv >= THRESHOLD_MV) {
    display.setTextColor(SENSAIR_YELLOW, SENSAIR_BLACK);
    display.print("SOUND!");
  } else {
    display.setTextColor(SENSAIR_DARKGREY, SENSAIR_BLACK);
    display.print("quiet ");
  }

  /* --- bar with peak hold --------------------------------------------- */
  float frac = lv.rmsMv / FULLSCALE_MV;
  if (frac > 1) frac = 1;
  int16_t fill = (int16_t)(barW * frac);
  uint16_t color = lv.rmsMv >= THRESHOLD_MV ? SENSAIR_YELLOW : SENSAIR_GREEN;
  display.fillRect(barX, barY, fill, barH, color);
  display.fillRect(barX + fill, barY, barW - fill, barH, SENSAIR_BLACK);

  if (frac >= peakHold || millis() > peakHoldUntil) {
    peakHold = frac;
    peakHoldUntil = millis() + 1500;
  }
  int16_t px = barX + (int16_t)(barW * peakHold);
  if (px >= barX + barW) px = barX + barW - 1;
  display.drawFastVLine(px, barY, barH, SENSAIR_RED);

  /* Serial Plotter friendly: rms and peak per line */
  Serial.printf("rms:%.1f peak:%.1f\n", lv.rmsMv, lv.peakMv);
}
