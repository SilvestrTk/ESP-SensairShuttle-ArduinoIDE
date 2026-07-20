/*
 * 11_WiFi_Setup — SensairShuttle library
 *
 * Complete on-device WiFi onboarding:
 *   1. scans for networks and lists them (strongest first, tap to select)
 *   2. on-screen keyboard to type the password
 *      (page key switches lowercase / UPPERCASE / symbols)
 *   3. connects and verifies real internet access with an HTTP request
 *      to a known connectivity-check endpoint (expects HTTP 204)
 *
 * Selecting an open network skips the keyboard. The result screen shows
 * IP, RSSI and the check round-trip time; AGAIN rescans.
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 */
#include <WiFi.h>
#include <SensairDisplay.h>
#include <SensairTouch.h>

SensairDisplay display;
SensairTouch touch;

const int16_t M = SENSAIR_LCD_MARGIN;

/* ---------------- connectivity check target ---------------- */
const char *CHECK_HOST = "connectivitycheck.gstatic.com";
const char *CHECK_PATH = "/generate_204";

/* ---------------- state machine ---------------- */
enum State { ST_SCAN, ST_LIST, ST_KEYBOARD, ST_CONNECT, ST_RESULT };
State state = ST_SCAN;

/* ---------------- scan results ---------------- */
const uint8_t MAX_NETS = 6;
struct Net {
  String ssid;
  int32_t rssi;
  bool open;
};
Net nets[MAX_NETS];
uint8_t netCount = 0;

String selSsid;
bool selOpen = false;
String password;
bool connectOk = false;
bool internetOk = false;
uint32_t pingMs = 0;

/* ---------------- keyboard geometry ---------------- */
const int16_t KEY_W = 25, KEY_H = 26, KEY_STEP = 26;  /* 10 * 26 = 260 wide */
const int16_t KB_TOP = 58, KEY_ROW_H = 30;
const int16_t SP_Y = 182, SP_H = 30;
const int16_t SP_W[5] = {34, 44, 76, 44, 50};         /* X, page, space, bksp, OK */
uint8_t kbPage = 0;

const char *KB_ROWS[3][4] = {
  {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"},
  {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"},
  {"!@#$%^&*()", "-_=+[]{}", ";:'\",.<>", "/\\?|~`"},
};
const char *PAGE_LABEL[3] = {"ABC", "#+=", "abc"};    /* label = what you switch TO */

/* key codes returned by keyAt(): printable chars, or: */
const int KEY_NONE = -1, KEY_PAGE = 1, KEY_BKSP = 2, KEY_OK = 3, KEY_CANCEL = 4;

int16_t rowX(uint8_t row) {
  int16_t n = strlen(KB_ROWS[kbPage][row]);
  return M + (10 * KEY_STEP - n * KEY_STEP) / 2;
}

int16_t spX(uint8_t i) {
  int16_t x = M;
  for (uint8_t k = 0; k < i; k++) x += SP_W[k] + 3;
  return x;
}

/* ---------------- small UI helpers ---------------- */

bool pressEvent(int16_t &tx, int16_t &ty) {
  /* one event per new finger contact, acting on touch-down for speed */
  static bool held = false;
  SensairTouchPoint p;
  bool down = touch.read(p) && p.touched;
  if (down && !held) {
    held = true;
    tx = p.x;
    ty = p.y;
    return true;
  }
  if (!down) held = false;
  return false;
}

bool inRect(int16_t px, int16_t py, int16_t x, int16_t y, int16_t w, int16_t h) {
  return px >= x && px < x + w && py >= y && py < y + h;
}

void button(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, uint16_t color) {
  display.fillRect(x, y, w, h, color);
  display.drawRect(x, y, w, h, SENSAIR_GREY);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, color);
  int16_t tw = strlen(label) * 12;
  display.setCursor(x + (w - tw) / 2, y + (h - 14) / 2);
  display.print(label);
}

void title(const char *t) {
  display.fillScreen(SENSAIR_BLACK);
  display.setTextSize(1);
  display.setCursor(M, M);
  display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  display.println(t);
}

/* ---------------- scan + list screen ---------------- */

void doScan() {
  title("WiFi setup");
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  display.setCursor(M, 60);
  display.print("scanning...");
  Serial.println("scanning...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int16_t found = WiFi.scanNetworks();

  /* keep the strongest MAX_NETS with a visible SSID */
  netCount = 0;
  for (uint8_t pick = 0; pick < MAX_NETS; pick++) {
    int16_t best = -1;
    for (int16_t i = 0; i < found; i++) {
      if (WiFi.SSID(i).length() == 0) continue;
      bool taken = false;
      for (uint8_t k = 0; k < netCount; k++) {
        if (nets[k].ssid == WiFi.SSID(i)) taken = true;
      }
      if (taken) continue;
      if (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best)) best = i;
    }
    if (best < 0) break;
    nets[netCount].ssid = WiFi.SSID(best);
    nets[netCount].rssi = WiFi.RSSI(best);
    nets[netCount].open = (WiFi.encryptionType(best) == WIFI_AUTH_OPEN);
    netCount++;
  }
  WiFi.scanDelete();
  Serial.printf("%d networks (showing %d)\n", found, netCount);
}

void drawList() {
  title("tap a network");
  button(display.width() - M - 70, M - 2, 70, 24, "SCAN", SENSAIR_NAVY);

  if (netCount == 0) {
    display.setTextSize(2);
    display.setTextColor(SENSAIR_ORANGE, SENSAIR_BLACK);
    display.setCursor(M, 80);
    display.print("no networks found");
    return;
  }
  for (uint8_t i = 0; i < netCount; i++) {
    int16_t y = 44 + i * 30;
    display.fillRect(M, y, display.width() - 2 * M, 28, SENSAIR_DARKGREY);
    display.drawRect(M, y, display.width() - 2 * M, 28, SENSAIR_GREY);
    display.setTextSize(2);
    display.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
    display.setCursor(M + 6, y + 7);
    String s = nets[i].ssid.substring(0, 14);
    display.print(s);
    display.setTextSize(1);
    display.setTextColor(nets[i].open ? SENSAIR_GREEN : SENSAIR_GREY, SENSAIR_DARKGREY);
    display.setCursor(display.width() - M - 52, y + 10);
    display.printf("%4d %s", nets[i].rssi, nets[i].open ? "open" : "lock");
  }
}

void listTouch(int16_t tx, int16_t ty) {
  if (inRect(tx, ty, display.width() - M - 70, M - 2, 70, 24)) {
    state = ST_SCAN;
    return;
  }
  for (uint8_t i = 0; i < netCount; i++) {
    if (inRect(tx, ty, M, 44 + i * 30, display.width() - 2 * M, 28)) {
      selSsid = nets[i].ssid;
      selOpen = nets[i].open;
      password = "";
      Serial.printf("selected: %s (%s)\n", selSsid.c_str(), selOpen ? "open" : "secured");
      state = selOpen ? ST_CONNECT : ST_KEYBOARD;
      return;
    }
  }
}

/* ---------------- keyboard screen ---------------- */

void drawPasswordField() {
  display.fillRect(M, 24, display.width() - 2 * M, 28, SENSAIR_DARKGREY);
  display.drawRect(M, 24, display.width() - 2 * M, 28, SENSAIR_GREY);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
  display.setCursor(M + 5, 31);
  /* show the tail when the password is longer than the field */
  String shown = password.length() > 20 ? password.substring(password.length() - 20) : password;
  display.print(shown);
  display.print("_ ");
}

void drawKey(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, uint16_t bg) {
  display.fillRect(x, y, w, h, bg);
  display.drawRect(x, y, w, h, SENSAIR_GREY);
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, bg);
  int16_t tw = strlen(label) * 12;
  display.setCursor(x + (w - tw) / 2, y + (h - 14) / 2);
  display.print(label);
}

void drawKeyboard() {
  title(("password for " + selSsid.substring(0, 22)).c_str());
  drawPasswordField();

  for (uint8_t r = 0; r < 4; r++) {
    const char *row = KB_ROWS[kbPage][r];
    for (uint8_t c = 0; c < strlen(row); c++) {
      char label[2] = {row[c], 0};
      drawKey(rowX(r) + c * KEY_STEP, KB_TOP + r * KEY_ROW_H, KEY_W, KEY_H,
              label, SENSAIR_DARKGREY);
    }
  }
  drawKey(spX(0), SP_Y, SP_W[0], SP_H, "X", SENSAIR_RED);
  drawKey(spX(1), SP_Y, SP_W[1], SP_H, PAGE_LABEL[kbPage], SENSAIR_NAVY);
  drawKey(spX(2), SP_Y, SP_W[2], SP_H, " ", SENSAIR_DARKGREY);
  drawKey(spX(3), SP_Y, SP_W[3], SP_H, "<", SENSAIR_NAVY);
  drawKey(spX(4), SP_Y, SP_W[4], SP_H, "OK", SENSAIR_GREEN);
}

int keyAt(int16_t tx, int16_t ty) {
  for (uint8_t r = 0; r < 4; r++) {
    const char *row = KB_ROWS[kbPage][r];
    int16_t y = KB_TOP + r * KEY_ROW_H;
    if (ty < y || ty >= y + KEY_H) continue;
    int16_t c = (tx - rowX(r)) / KEY_STEP;
    if (c >= 0 && c < (int16_t)strlen(row)) return row[c];
  }
  if (ty >= SP_Y && ty < SP_Y + SP_H) {
    if (inRect(tx, ty, spX(0), SP_Y, SP_W[0], SP_H)) return KEY_CANCEL;
    if (inRect(tx, ty, spX(1), SP_Y, SP_W[1], SP_H)) return KEY_PAGE;
    if (inRect(tx, ty, spX(2), SP_Y, SP_W[2], SP_H)) return ' ';
    if (inRect(tx, ty, spX(3), SP_Y, SP_W[3], SP_H)) return KEY_BKSP;
    if (inRect(tx, ty, spX(4), SP_Y, SP_W[4], SP_H)) return KEY_OK;
  }
  return KEY_NONE;
}

void keyboardTouch(int16_t tx, int16_t ty) {
  int k = keyAt(tx, ty);
  switch (k) {
    case KEY_NONE:
      return;
    case KEY_CANCEL:
      state = ST_LIST;
      return;
    case KEY_PAGE:
      kbPage = (kbPage + 1) % 3;
      drawKeyboard();
      return;
    case KEY_BKSP:
      if (password.length()) password.remove(password.length() - 1);
      drawPasswordField();
      return;
    case KEY_OK:
      state = ST_CONNECT;
      return;
    default:
      if (password.length() < 63) password += (char)k;
      drawPasswordField();
      return;
  }
}

/* ---------------- connect + verify ---------------- */

bool checkInternet(uint32_t &ms) {
  WiFiClient client;
  uint32_t t0 = millis();
  if (!client.connect(CHECK_HOST, 80)) return false;
  client.printf("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                CHECK_PATH, CHECK_HOST);
  uint32_t deadline = millis() + 5000;
  while (!client.available() && millis() < deadline) delay(10);
  String status = client.readStringUntil('\n');
  ms = millis() - t0;
  client.stop();
  Serial.printf("HTTP: %s\n", status.c_str());
  return status.indexOf("204") > 0;
}

void doConnect() {
  title("connecting");
  display.setTextSize(2);
  display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  display.setCursor(M, 50);
  display.print(selSsid.substring(0, 21));
  Serial.printf("connecting to %s ...\n", selSsid.c_str());

  WiFi.disconnect();
  delay(100);
  if (selOpen) WiFi.begin(selSsid.c_str());
  else WiFi.begin(selSsid.c_str(), password.c_str());

  uint32_t deadline = millis() + 15000;
  display.setCursor(M, 90);
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
    display.print(".");
    delay(400);
  }
  connectOk = (WiFi.status() == WL_CONNECTED);

  internetOk = false;
  pingMs = 0;
  if (connectOk) {
    Serial.printf("connected, IP %s\n", WiFi.localIP().toString().c_str());
    display.setCursor(M, 120);
    display.setTextColor(SENSAIR_CYAN, SENSAIR_BLACK);
    display.printf("checking %s", CHECK_HOST);
    internetOk = checkInternet(pingMs);
  } else {
    Serial.println("connect failed");
  }
  state = ST_RESULT;
}

void drawResult() {
  title("result");
  display.setTextSize(2);
  display.setCursor(M, 40);

  if (connectOk) {
    display.setTextColor(SENSAIR_GREEN, SENSAIR_BLACK);
    display.printf("connected\n");
    display.setTextSize(1);
    display.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
    display.setCursor(M, 68);
    display.printf("SSID: %s\n", selSsid.c_str());
    display.setCursor(M, 80);
    display.printf("IP:   %s   RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    display.setTextSize(2);
    display.setCursor(M, 100);
    if (internetOk) {
      display.setTextColor(SENSAIR_GREEN, SENSAIR_BLACK);
      display.printf("internet OK, %lu ms", (unsigned long)pingMs);
    } else {
      display.setTextColor(SENSAIR_ORANGE, SENSAIR_BLACK);
      display.print("no internet access");
    }
    Serial.printf("internet: %s (%lu ms)\n", internetOk ? "OK" : "FAIL", (unsigned long)pingMs);
  } else {
    display.setTextColor(SENSAIR_RED, SENSAIR_BLACK);
    display.print("connect failed\n");
    display.setTextSize(1);
    display.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
    display.setCursor(M, 70);
    display.print("wrong password? out of range?");
  }

  if (connectOk) {
    button(display.width() / 2 - 50, 150, 100, 34, "AGAIN", SENSAIR_NAVY);
  } else {
    button(M + 20, 150, 100, 34, "RETRY", SENSAIR_NAVY);
    button(display.width() - M - 120, 150, 100, 34, "BACK", SENSAIR_DARKGREY);
  }
}

void resultTouch(int16_t tx, int16_t ty) {
  if (connectOk) {
    if (inRect(tx, ty, display.width() / 2 - 50, 150, 100, 34)) state = ST_SCAN;
  } else {
    if (inRect(tx, ty, M + 20, 150, 100, 34)) state = ST_KEYBOARD;
    if (inRect(tx, ty, display.width() - M - 120, 150, 100, 34)) state = ST_LIST;
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
}

void loop() {
  static State drawn = ST_RESULT;   /* force first draw */

  /* blocking states */
  if (state == ST_SCAN) {
    doScan();
    state = ST_LIST;
    drawn = ST_SCAN;
  } else if (state == ST_CONNECT) {
    doConnect();
    drawn = ST_CONNECT;
  }

  /* draw the current interactive screen once */
  if (drawn != state) {
    if (state == ST_LIST) drawList();
    else if (state == ST_KEYBOARD) drawKeyboard();
    else if (state == ST_RESULT) drawResult();
    drawn = state;
  }

  int16_t tx, ty;
  if (pressEvent(tx, ty)) {
    if (state == ST_LIST) listTouch(tx, ty);
    else if (state == ST_KEYBOARD) keyboardTouch(tx, ty);
    else if (state == ST_RESULT) resultTouch(tx, ty);
  }
  delay(5);
}
