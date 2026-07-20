/*
 * 14_Rotary_Keyboard — SensairShuttle library
 *
 * Space-saving text entry with a rotating character picker:
 *
 *   - UP / DOWN arrows rotate through the current character set
 *     (hold an arrow to auto-repeat; the arrows preview the character
 *     they lead to)
 *   - tapping the big character in the middle appends it to the text
 *   - MODE cycles the set: abc -> ABC -> 123 -> #?! (each set also
 *     contains the space character, shown as "spc")
 *   - "<" deletes the last character (backspace)
 *   - X cancels the whole entry, OK confirms it
 *
 * The confirmed text is printed to Serial; tap the result screen to
 * start over.
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 */
#include <SensairDisplay.h>
#include <SensairTouch.h>

SensairDisplay display;
SensairTouch touch;

const int16_t M = SENSAIR_LCD_MARGIN;

/* ---------------- character sets ---------------- */
const char *SETS[4] = {
  "abcdefghijklmnopqrstuvwxyz ",
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ ",
  "0123456789 ",
  "!@#$%^&*()-_=+[]{};:'\",.<>/?\\|~` ",
};
const char *SET_NAMES[4] = {"abc", "ABC", "123", "#?!"};
uint8_t setIdx = 0;
uint8_t charIdx = 0;

String text;
bool done = false;

/* ---------------- layout ---------------- */
/* rotator column (left-center) */
const int16_t ROT_X = 56, ROT_W = 64;
const int16_t UP_Y = 52, UP_H = 38;
const int16_t CH_Y = 96, CH_H = 56;
const int16_t DN_Y = 158, DN_H = 38;
/* action buttons column (right) */
const int16_t BT_X = 160, BT_W = 110, BT_H = 34;
const int16_t MODE_Y = 52, BKSP_Y = 92, CANCEL_Y = 148, OK_Y = 188;

/* ---------------- helpers ---------------- */

char curChar()  { return SETS[setIdx][charIdx]; }
uint8_t setLen() { return strlen(SETS[setIdx]); }

void triangle(int16_t cx, int16_t y, int16_t halfW, int16_t h, bool up, uint16_t c) {
  for (int16_t i = 0; i < h; i++) {
    int16_t w = up ? (halfW * i / h) : (halfW * (h - 1 - i) / h);
    display.drawFastHLine(cx - w, y + i, 2 * w + 1, c);
  }
}

void drawGlyphBig(int16_t x, int16_t y, int16_t w, int16_t h, char c,
                  uint16_t fg, uint16_t bg) {
  /* size-4 character centered in the box; space shown as "spc" */
  if (c == ' ') {
    display.setTextSize(2);
    display.setTextColor(SENSAIR_GREY, bg);
    display.setCursor(x + (w - 36) / 2, y + (h - 14) / 2);
    display.print("spc");
  } else {
    display.setTextSize(4);
    display.setTextColor(fg, bg);
    display.setCursor(x + (w - 24) / 2 + 2, y + (h - 32) / 2 + 2);
    display.print(c);
  }
}

void drawArrows() {
  uint8_t n = setLen();
  char prev = SETS[setIdx][(charIdx + n - 1) % n];
  char next = SETS[setIdx][(charIdx + 1) % n];

  display.fillRect(ROT_X, UP_Y, ROT_W, UP_H, SENSAIR_DARKGREY);
  display.drawRect(ROT_X, UP_Y, ROT_W, UP_H, SENSAIR_GREY);
  triangle(ROT_X + 18, UP_Y + 8, 10, 14, true, SENSAIR_WHITE);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_GREY, SENSAIR_DARKGREY);
  display.setCursor(ROT_X + 40, UP_Y + 11);
  display.print(prev == ' ' ? '_' : prev);

  display.fillRect(ROT_X, DN_Y, ROT_W, DN_H, SENSAIR_DARKGREY);
  display.drawRect(ROT_X, DN_Y, ROT_W, DN_H, SENSAIR_GREY);
  triangle(ROT_X + 18, DN_Y + 16, 10, 14, false, SENSAIR_WHITE);
  display.setCursor(ROT_X + 40, DN_Y + 11);
  display.print(next == ' ' ? '_' : next);
}

void drawChar() {
  display.fillRect(ROT_X, CH_Y, ROT_W, CH_H, SENSAIR_NAVY);
  display.drawRect(ROT_X, CH_Y, ROT_W, CH_H, SENSAIR_CYAN);
  drawGlyphBig(ROT_X, CH_Y, ROT_W, CH_H, curChar(), SENSAIR_WHITE, SENSAIR_NAVY);
  drawArrows();  /* previews change together with the character */
}

void actionButton(int16_t y, const char *label, uint16_t color) {
  display.fillRect(BT_X, y, BT_W, BT_H, color);
  display.drawRect(BT_X, y, BT_W, BT_H, SENSAIR_GREY);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, color);
  display.setCursor(BT_X + (BT_W - strlen(label) * 12) / 2, y + 10);
  display.print(label);
}

void drawField() {
  display.fillRect(M, M, display.width() - 2 * M, 26, SENSAIR_DARKGREY);
  display.drawRect(M, M, display.width() - 2 * M, 26, SENSAIR_GREY);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
  display.setCursor(M + 5, M + 6);
  String shown = text.length() > 19 ? text.substring(text.length() - 19) : text;
  display.print(shown);
  display.print("_");
}

void drawEditor() {
  display.fillScreen(SENSAIR_BLACK);
  drawField();
  drawChar();
  actionButton(MODE_Y, SET_NAMES[setIdx], SENSAIR_NAVY);
  actionButton(BKSP_Y, "<", SENSAIR_DARKGREY);
  actionButton(CANCEL_Y, "X", SENSAIR_RED);
  actionButton(OK_Y, "OK", SENSAIR_GREEN);
}

void drawDone(bool confirmed) {
  display.fillScreen(SENSAIR_BLACK);
  display.setTextSize(2);
  display.setCursor(M, 40);
  if (confirmed) {
    display.setTextColor(SENSAIR_GREEN, SENSAIR_BLACK);
    display.println("confirmed:");
    display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
    display.setCursor(M, 70);
    display.setTextWrap(true);
    display.print(text.length() ? text : "(empty)");
  } else {
    display.setTextColor(SENSAIR_ORANGE, SENSAIR_BLACK);
    display.println("cancelled");
  }
  display.setTextSize(1);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.setCursor(M, display.height() - M - 10);
  display.print("tap to start again");
}

/* ---------------- zones ---------------- */

/* plain ints, not an enum type, so the Arduino auto-prototypes compile */
const int Z_NONE = 0, Z_UP = 1, Z_DOWN = 2, Z_CHAR = 3, Z_MODE = 4,
          Z_BKSP = 5, Z_CANCEL = 6, Z_OK = 7;

int zoneAt(int16_t x, int16_t y) {
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

void act(int z) {
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
      if (text.length() < 60) text += curChar();
      drawField();
      Serial.printf("text: \"%s\"\n", text.c_str());
      break;
    case Z_MODE:
      setIdx = (setIdx + 1) % 4;
      charIdx = 0;
      actionButton(MODE_Y, SET_NAMES[setIdx], SENSAIR_NAVY);
      drawChar();
      break;
    case Z_BKSP:
      if (text.length()) text.remove(text.length() - 1);
      drawField();
      break;
    case Z_CANCEL:
      Serial.println("cancelled");
      text = "";
      done = true;
      drawDone(false);
      break;
    case Z_OK:
      Serial.printf("confirmed: \"%s\"\n", text.c_str());
      done = true;
      drawDone(true);
      break;
    default:
      break;
  }
}

/* ---------------- main ---------------- */

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setRotation(1);
  if (!touch.begin()) {
    Serial.println("touch controller not found!");
    while (true) delay(1000);
  }
  touch.setRotation(display.rotation());

  drawEditor();
}

void loop() {
  static int held = Z_NONE;
  static bool contact = false;
  static uint32_t nextRepeat = 0;

  SensairTouchPoint p;
  bool down = touch.read(p) && p.touched;

  if (done) {
    /* result screen: tap anywhere to restart */
    if (down && !contact) {
      done = false;
      text = "";
      setIdx = 0;
      charIdx = 0;
      drawEditor();
    }
    contact = down;
    delay(5);
    return;
  }

  if (down && !contact) {
    /* new contact */
    held = zoneAt(p.x, p.y);
    act(held);
    nextRepeat = millis() + 400;   /* auto-repeat delay for the arrows */
  } else if (down && (held == Z_UP || held == Z_DOWN)) {
    /* hold-to-repeat on the arrows */
    if (millis() >= nextRepeat) {
      act(held);
      nextRepeat = millis() + 120;
    }
  } else if (!down) {
    held = Z_NONE;
  }
  contact = down;

  delay(5);
}
