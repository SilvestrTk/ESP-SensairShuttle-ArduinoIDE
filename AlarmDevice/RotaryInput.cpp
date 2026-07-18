/*
 * RotaryInput.cpp — rotating-character text entry for the Alarm Device.
 */
#include "RotaryInput.h"
#include "App.h"          /* gDisplay, gTouch */
#include "DebugLog.h"

namespace RotaryInput {

static const int16_t M = SENSAIR_LCD_MARGIN;

static const char *SETS[4] = {
  "abcdefghijklmnopqrstuvwxyz ",
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ ",
  "0123456789 ",
  "!@#$%^&*()-_=+[]{};:'\",.<>/?\\|~` ",
};
static const char *SET_NAMES[4] = {"abc", "ABC", "123", "#?!"};

/* layout */
static const int16_t ROT_X = 56, ROT_W = 64;
static const int16_t UP_Y = 52, UP_H = 38;
static const int16_t CH_Y = 96, CH_H = 56;
static const int16_t DN_Y = 158, DN_H = 38;
static const int16_t BT_X = 160, BT_W = 110, BT_H = 34;
static const int16_t MODE_Y = 52, BKSP_Y = 92, CANCEL_Y = 148, OK_Y = 188;

static const int Z_NONE = 0, Z_UP = 1, Z_DOWN = 2, Z_CHAR = 3, Z_MODE = 4,
                 Z_BKSP = 5, Z_CANCEL = 6, Z_OK = 7;

static uint8_t setIdx, charIdx;
static String buf;
static const char *titleText;
static size_t maxLength;

static char curChar() { return SETS[setIdx][charIdx]; }
static uint8_t setLen() { return strlen(SETS[setIdx]); }

static void triangle(int16_t cx, int16_t y, int16_t halfW, int16_t h, bool up, uint16_t c) {
  for (int16_t i = 0; i < h; i++) {
    int16_t w = up ? (halfW * i / h) : (halfW * (h - 1 - i) / h);
    gDisplay.drawFastHLine(cx - w, y + i, 2 * w + 1, c);
  }
}

static void drawArrows() {
  uint8_t n = setLen();
  char prev = SETS[setIdx][(charIdx + n - 1) % n];
  char next = SETS[setIdx][(charIdx + 1) % n];

  gDisplay.fillRect(ROT_X, UP_Y, ROT_W, UP_H, SENSAIR_DARKGREY);
  gDisplay.drawRect(ROT_X, UP_Y, ROT_W, UP_H, SENSAIR_GREY);
  triangle(ROT_X + 18, UP_Y + 8, 10, 14, true, SENSAIR_WHITE);
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_GREY, SENSAIR_DARKGREY);
  gDisplay.setCursor(ROT_X + 40, UP_Y + 11);
  gDisplay.print(prev == ' ' ? '_' : prev);

  gDisplay.fillRect(ROT_X, DN_Y, ROT_W, DN_H, SENSAIR_DARKGREY);
  gDisplay.drawRect(ROT_X, DN_Y, ROT_W, DN_H, SENSAIR_GREY);
  triangle(ROT_X + 18, DN_Y + 16, 10, 14, false, SENSAIR_WHITE);
  gDisplay.setCursor(ROT_X + 40, DN_Y + 11);
  gDisplay.print(next == ' ' ? '_' : next);
}

static void drawChar() {
  gDisplay.fillRect(ROT_X, CH_Y, ROT_W, CH_H, SENSAIR_NAVY);
  gDisplay.drawRect(ROT_X, CH_Y, ROT_W, CH_H, SENSAIR_CYAN);
  char c = curChar();
  if (c == ' ') {
    gDisplay.setTextSize(2);
    gDisplay.setTextColor(SENSAIR_GREY, SENSAIR_NAVY);
    gDisplay.setCursor(ROT_X + (ROT_W - 36) / 2, CH_Y + (CH_H - 14) / 2);
    gDisplay.print("spc");
  } else {
    gDisplay.setTextSize(4);
    gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_NAVY);
    gDisplay.setCursor(ROT_X + (ROT_W - 24) / 2 + 2, CH_Y + (CH_H - 32) / 2 + 2);
    gDisplay.print(c);
  }
  drawArrows();
}

static void actionButton(int16_t y, const char *label, uint16_t color) {
  gDisplay.fillRect(BT_X, y, BT_W, BT_H, color);
  gDisplay.drawRect(BT_X, y, BT_W, BT_H, SENSAIR_GREY);
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_WHITE, color);
  gDisplay.setCursor(BT_X + (BT_W - (int16_t)strlen(label) * 12) / 2, y + 10);
  gDisplay.print(label);
}

static void drawField() {
  gDisplay.fillRect(M, M + 12, gDisplay.width() - 2 * M, 26, SENSAIR_DARKGREY);
  gDisplay.drawRect(M, M + 12, gDisplay.width() - 2 * M, 26, SENSAIR_GREY);
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
  gDisplay.setCursor(M + 5, M + 18);
  String shown = buf.length() > 19 ? buf.substring(buf.length() - 19) : buf;
  gDisplay.print(shown);
  gDisplay.print("_");
}

static void drawAll() {
  gDisplay.fillScreen(SENSAIR_BLACK);
  gDisplay.setTextSize(1);
  gDisplay.setCursor(M, M);
  gDisplay.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  gDisplay.print(titleText);
  drawField();
  drawChar();
  actionButton(MODE_Y, SET_NAMES[setIdx], SENSAIR_NAVY);
  actionButton(BKSP_Y, "<", SENSAIR_DARKGREY);
  actionButton(CANCEL_Y, "X", SENSAIR_RED);
  actionButton(OK_Y, "OK", SENSAIR_GREEN);
}

static int zoneAt(int16_t x, int16_t y) {
  if (x >= ROT_X && x < ROT_X + ROT_W) {
    if (y >= UP_Y && y < UP_Y + UP_H) return Z_UP;
    if (y >= CH_Y && y < CH_Y + CH_H) return Z_CHAR;
    if (y >= DN_Y && y < DN_Y + DN_H) return Z_DOWN;
  }
  if (x >= BT_X && x < BT_X + BT_W) {
    if (y >= MODE_Y && y < MODE_Y + BT_H) return Z_MODE;
    if (y >= BKSP_Y && y < BKSP_Y + BT_H) return Z_BKSP;
    if (y >= CANCEL_Y && y < CANCEL_Y + BT_H) return Z_CANCEL;
    if (y >= OK_Y && y < OK_Y + BT_H) return Z_OK;
  }
  return Z_NONE;
}

/* returns: 0 continue, 1 confirmed, -1 cancelled */
static int act(int z) {
  switch (z) {
    case Z_UP:
      charIdx = (charIdx + setLen() - 1) % setLen();
      drawChar();
      break;
    case Z_DOWN:
      charIdx = (charIdx + 1) % setLen();
      drawChar();
      break;
    case Z_CHAR:
      if (buf.length() < maxLength) buf += curChar();
      drawField();
      break;
    case Z_MODE:
      setIdx = (setIdx + 1) % 4;
      charIdx = 0;
      actionButton(MODE_Y, SET_NAMES[setIdx], SENSAIR_NAVY);
      drawChar();
      break;
    case Z_BKSP:
      if (buf.length()) buf.remove(buf.length() - 1);
      drawField();
      break;
    case Z_CANCEL:
      return -1;
    case Z_OK:
      return 1;
  }
  return 0;
}

bool edit(const char *title, String &value, size_t maxLen, void (*bgTick)()) {
  titleText = title;
  buf = value;
  maxLength = maxLen;
  setIdx = 0;
  charIdx = 0;
  drawAll();
  LOGI("UI", "rotary input: %s", title);

  int held = Z_NONE;
  bool contact = false;
  uint32_t nextRepeat = 0;

  while (true) {
    if (bgTick) bgTick();

    SensairTouchPoint p;
    bool down = gTouch.read(p) && p.touched;

    if (down && !contact) {
      held = zoneAt(p.x, p.y);
      int r = act(held);
      if (r != 0) {
        if (r > 0) value = buf;
        LOGI("UI", "rotary input %s: \"%s\"", r > 0 ? "OK" : "cancelled",
             r > 0 ? value.c_str() : buf.c_str());
        return r > 0;
      }
      nextRepeat = millis() + 400;
    } else if (down && (held == Z_UP || held == Z_DOWN)) {
      if ((int32_t)(millis() - nextRepeat) >= 0) {
        act(held);
        nextRepeat = millis() + 120;
      }
    } else if (!down) {
      held = Z_NONE;
    }
    contact = down;
    delay(5);
  }
}

}  // namespace RotaryInput
