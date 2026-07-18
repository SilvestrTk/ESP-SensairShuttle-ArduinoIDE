/*
 * Ui.cpp — touchscreen user interface of the Alarm Device.
 */
#include "Ui.h"
#include "App.h"
#include "AppConfig.h"
#include "DebugLog.h"
#include "AlarmTypes.h"
#include "Storage.h"
#include "Sensors.h"
#include "AlarmManager.h"
#include "AlarmSiren.h"
#include "AlarmNet.h"
#include "RotaryInput.h"
#include <WiFi.h>

namespace Ui {

static const int16_t M = SENSAIR_LCD_MARGIN;

enum Screen : uint8_t { SCR_MAIN, SCR_ALARMS, SCR_EDIT, SCR_CONN, SCR_WIFI };
static Screen screen = SCR_MAIN;
static bool needRedraw = true;
static uint32_t nextRefresh = 0;
static bool stopShown = false;

/* alarm editor working copy */
static uint8_t editId = 0;
static AlarmSettings editCfg;

/* wifi scan results */
static const uint8_t MAX_NETS = 6;
static String netSsid[MAX_NETS];
static int32_t netRssi[MAX_NETS];
static bool netOpen[MAX_NETS];
static uint8_t netCount = 0;

/* editor hold-repeat state */
static int heldZone = 0;
static uint32_t nextRepeat = 0;

/* ---------------- shared widgets ---------------- */

static void button(int16_t x, int16_t y, int16_t w, int16_t h,
                   const char *label, uint16_t color) {
  gDisplay.fillRect(x, y, w, h, color);
  gDisplay.drawRect(x, y, w, h, SENSAIR_GREY);
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_WHITE, color);
  gDisplay.setCursor(x + (w - (int16_t)strlen(label) * 12) / 2, y + (h - 14) / 2);
  gDisplay.print(label);
}

static bool inRect(int16_t px, int16_t py, int16_t x, int16_t y, int16_t w, int16_t h) {
  return px >= x && px < x + w && py >= y && py < y + h;
}

static void title(const char *t) {
  gDisplay.fillScreen(SENSAIR_BLACK);
  gDisplay.fillRect(0, 0, gDisplay.width(), 24, SENSAIR_NAVY);
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_NAVY);
  gDisplay.setCursor(M + 8, 5);
  gDisplay.print(t);
}

static void fmtValue(char *buf, size_t n, uint8_t id, float v, bool valid) {
  if (!valid) {
    snprintf(buf, n, "n/a");
  } else {
    snprintf(buf, n, "%.*f", ALARM_DEFS[id].decimals, v);
  }
}

/* ---------------- MAIN screen ---------------- */

static const int16_t ROW_Y0 = 30, ROW_H = 22;
static const int16_t BTN_Y = 152, BTN_H = 38;
static const int16_t STOP_Y = 148, STOP_H = 84;

static void drawMainValues() {
  for (uint8_t i = 0; i < ALARM_COUNT; i++) {
    int16_t y = ROW_Y0 + i * ROW_H;
    const AlarmSettings &s = Storage::alarmCfg(i);
    const AlarmRuntime &r = AlarmManager::runtime(i);
    bool valid = false;
    float v = Sensors::valueFor(i, valid);

    char vs[24];
    fmtValue(vs, sizeof(vs), i, v, valid);
    const char *unit = valid ? ALARM_DEFS[i].unit : "";
    if (i == ALARM_AIR && valid) unit = Sensors::iaqLevelName(v);

    gDisplay.setTextSize(2);
    gDisplay.setTextColor(valid ? SENSAIR_WHITE : SENSAIR_DARKGREY, SENSAIR_BLACK);
    gDisplay.setCursor(104, y);
    gDisplay.printf("%s %s        ", vs, unit);

    /* alarm state marker */
    gDisplay.setTextSize(1);
    gDisplay.setCursor(238, y + 4);
    if (r.localActive || r.overThreshold) {
      gDisplay.setTextColor(SENSAIR_RED, SENSAIR_BLACK);
      gDisplay.print("!");
    } else {
      gDisplay.print(" ");
    }
    gDisplay.setTextColor((s.localEnabled || s.remoteEnabled) ? SENSAIR_GREEN
                                                              : SENSAIR_DARKGREY,
                          SENSAIR_BLACK);
    gDisplay.setCursor(248, y + 4);
    gDisplay.printf("%c%c", s.localEnabled ? 'L' : '-', s.remoteEnabled ? 'R' : '-');
  }
}

static void drawStatusLine() {
  gDisplay.setTextSize(1);
  gDisplay.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  gDisplay.setCursor(M, gDisplay.height() - M - 20);
  if (AlarmNet::connected()) {
    gDisplay.printf("WiFi: %s  %s          ", Storage::wifiSsid().c_str(),
                    AlarmNet::ip().c_str());
  } else {
    gDisplay.printf("WiFi: not connected                ");
  }
  gDisplay.setCursor(M, gDisplay.height() - M - 8);
  gDisplay.printf("ID: %s   unsent: %u    ", Storage::deviceId().c_str(),
                  AlarmNet::pendingCount());
}

static void drawStopOverlay() {
  button(M, STOP_Y, gDisplay.width() - 2 * M, STOP_H, "STOP ALARM", SENSAIR_RED);
  stopShown = true;
}

static void drawMain() {
  title("Alarm Device");
  gDisplay.setTextSize(1);
  gDisplay.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  for (uint8_t i = 0; i < ALARM_COUNT; i++) {
    gDisplay.setCursor(M, ROW_Y0 + i * ROW_H + 4);
    gDisplay.print(ALARM_DEFS[i].shortName);
  }
  drawMainValues();
  stopShown = false;
  if (AlarmManager::anyLocalActive()) {
    drawStopOverlay();
  } else {
    button(M, BTN_Y, 122, BTN_H, "ALARMS", SENSAIR_NAVY);
    button(M + 132, BTN_Y, 122, BTN_H, "CONNECT", SENSAIR_DARKGREY);
    drawStatusLine();
  }
}

static void touchMain(int16_t x, int16_t y) {
  if (stopShown) {
    if (inRect(x, y, M, STOP_Y, gDisplay.width() - 2 * M, STOP_H)) {
      AlarmManager::stopAllLocal();
      needRedraw = true;
    }
    return;
  }
  if (inRect(x, y, M, BTN_Y, 122, BTN_H)) {
    screen = SCR_ALARMS;
    needRedraw = true;
  } else if (inRect(x, y, M + 132, BTN_Y, 122, BTN_H)) {
    screen = SCR_CONN;
    needRedraw = true;
  }
}

/* ---------------- ALARM LIST screen ---------------- */

static const int16_t LROW_Y0 = 32, LROW_H = 30;

static void drawAlarmList() {
  title("Alarms");
  for (uint8_t i = 0; i < ALARM_COUNT; i++) {
    int16_t y = LROW_Y0 + i * LROW_H;
    const AlarmSettings &s = Storage::alarmCfg(i);
    gDisplay.fillRect(M, y, gDisplay.width() - 2 * M, LROW_H - 3, SENSAIR_DARKGREY);
    gDisplay.drawRect(M, y, gDisplay.width() - 2 * M, LROW_H - 3, SENSAIR_GREY);
    gDisplay.setTextSize(1);
    gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
    gDisplay.setCursor(M + 6, y + 9);
    gDisplay.print(ALARM_DEFS[i].shortName);
    gDisplay.setCursor(M + 90, y + 9);
    if (s.localEnabled || s.remoteEnabled) {
      gDisplay.setTextColor(SENSAIR_GREEN, SENSAIR_DARKGREY);
      gDisplay.printf("> %.*f %s  %s%s  reset %um", ALARM_DEFS[i].decimals,
                      s.threshold, ALARM_DEFS[i].unit,
                      s.localEnabled ? "L" : "", s.remoteEnabled ? "R" : "",
                      s.resetMinutes);
    } else {
      gDisplay.setTextColor(SENSAIR_GREY, SENSAIR_DARKGREY);
      gDisplay.print("off");
    }
  }
  button(M, 192, 100, 34, "BACK", SENSAIR_DARKGREY);
}

static void touchAlarmList(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < ALARM_COUNT; i++) {
    if (inRect(x, y, M, LROW_Y0 + i * LROW_H, gDisplay.width() - 2 * M, LROW_H - 3)) {
      editId = i;
      editCfg = Storage::alarmCfg(i);
      screen = SCR_EDIT;
      needRedraw = true;
      return;
    }
  }
  if (inRect(x, y, M, 192, 100, 34)) {
    screen = SCR_MAIN;
    needRedraw = true;
  }
}

/* ---------------- ALARM EDIT screen ---------------- */

static const int16_t THR_Y = 34, TOG1_Y = 74, TOG2_Y = 104, RST_Y = 140, EBTN_Y = 190;
static const int Z_THR_MINUS = 1, Z_THR_PLUS = 2, Z_RST_MINUS = 3, Z_RST_PLUS = 4;

static void drawEditValues() {
  const AlarmDef &d = ALARM_DEFS[editId];
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  gDisplay.setCursor(M + 46, THR_Y + 8);
  gDisplay.printf("%.*f %s    ", d.decimals, editCfg.threshold, d.unit);
  gDisplay.setCursor(M + 46, RST_Y + 8);
  gDisplay.printf("%u min    ", editCfg.resetMinutes);
}

static void drawCheckbox(int16_t y, const char *label, bool checked) {
  gDisplay.drawRect(M, y, 22, 22, SENSAIR_GREY);
  gDisplay.fillRect(M + 3, y + 3, 16, 16, checked ? SENSAIR_GREEN : SENSAIR_BLACK);
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  gDisplay.setCursor(M + 32, y + 4);
  gDisplay.print(label);
}

static void drawEdit() {
  title(ALARM_DEFS[editId].name);
  gDisplay.setTextSize(1);
  gDisplay.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  gDisplay.setCursor(M, THR_Y - 2);
  gDisplay.print("threshold (alarm when above)");
  button(M, THR_Y + 2, 34, 30, "-", SENSAIR_NAVY);
  button(gDisplay.width() - M - 34, THR_Y + 2, 34, 30, "+", SENSAIR_NAVY);
  drawCheckbox(TOG1_Y, "Local siren", editCfg.localEnabled);
  drawCheckbox(TOG2_Y, "Remote webhook", editCfg.remoteEnabled);
  gDisplay.setTextSize(1);
  gDisplay.setTextColor(SENSAIR_GREY, SENSAIR_BLACK);
  gDisplay.setCursor(M, RST_Y - 2);
  gDisplay.print("remote reset time");
  button(M, RST_Y + 2, 34, 30, "-", SENSAIR_NAVY);
  button(gDisplay.width() - M - 34, RST_Y + 2, 34, 30, "+", SENSAIR_NAVY);
  drawEditValues();
  button(M, EBTN_Y, 110, 36, "SAVE", SENSAIR_GREEN);
  button(gDisplay.width() - M - 110, EBTN_Y, 110, 36, "CANCEL", SENSAIR_DARKGREY);
}

static void editStep(int zone) {
  const AlarmDef &d = ALARM_DEFS[editId];
  switch (zone) {
    case Z_THR_MINUS:
      editCfg.threshold = max(d.min, editCfg.threshold - d.step);
      break;
    case Z_THR_PLUS:
      editCfg.threshold = min(d.max, editCfg.threshold + d.step);
      break;
    case Z_RST_MINUS:
      if (editCfg.resetMinutes > 1) editCfg.resetMinutes--;
      break;
    case Z_RST_PLUS:
      if (editCfg.resetMinutes < 240) editCfg.resetMinutes++;
      break;
  }
  drawEditValues();
}

static int editZoneAt(int16_t x, int16_t y) {
  if (inRect(x, y, M, THR_Y + 2, 34, 30)) return Z_THR_MINUS;
  if (inRect(x, y, gDisplay.width() - M - 34, THR_Y + 2, 34, 30)) return Z_THR_PLUS;
  if (inRect(x, y, M, RST_Y + 2, 34, 30)) return Z_RST_MINUS;
  if (inRect(x, y, gDisplay.width() - M - 34, RST_Y + 2, 34, 30)) return Z_RST_PLUS;
  return 0;
}

static void touchEdit(int16_t x, int16_t y) {
  int z = editZoneAt(x, y);
  if (z) {
    heldZone = z;
    nextRepeat = millis() + 400;
    editStep(z);
    return;
  }
  if (inRect(x, y, M, TOG1_Y, 220, 24)) {
    editCfg.localEnabled = !editCfg.localEnabled;
    drawCheckbox(TOG1_Y, "Local siren", editCfg.localEnabled);
  } else if (inRect(x, y, M, TOG2_Y, 220, 24)) {
    editCfg.remoteEnabled = !editCfg.remoteEnabled;
    drawCheckbox(TOG2_Y, "Remote webhook", editCfg.remoteEnabled);
  } else if (inRect(x, y, M, EBTN_Y, 110, 36)) {
    Storage::alarmCfg(editId) = editCfg;
    Storage::saveAlarm(editId);
    LOGI("UI", "alarm %u updated: thr=%.1f L=%d R=%d reset=%u", editId,
         editCfg.threshold, editCfg.localEnabled, editCfg.remoteEnabled,
         editCfg.resetMinutes);
    AlarmSiren::chirp();
    screen = SCR_ALARMS;
    needRedraw = true;
  } else if (inRect(x, y, gDisplay.width() - M - 110, EBTN_Y, 110, 36)) {
    screen = SCR_ALARMS;
    needRedraw = true;
  }
}

/* ---------------- CONNECTION screen ---------------- */

static const int16_t CROW_Y0 = 32, CROW_H = 34;

static void drawConn() {
  title("Connection");
  const char *labels[4] = {"WiFi", "Server URL", "Device ID", "URL -> default"};
  String values[4];
  values[0] = Storage::wifiSsid().length()
                  ? Storage::wifiSsid() + (AlarmNet::connected() ? " (ok)" : " (down)")
                  : String("not set");
  values[1] = Storage::serverUrl().substring(0, 24);
  values[2] = Storage::deviceId();
  values[3] = "";

  for (uint8_t i = 0; i < 4; i++) {
    int16_t y = CROW_Y0 + i * CROW_H;
    gDisplay.fillRect(M, y, gDisplay.width() - 2 * M, CROW_H - 4, SENSAIR_DARKGREY);
    gDisplay.drawRect(M, y, gDisplay.width() - 2 * M, CROW_H - 4, SENSAIR_GREY);
    gDisplay.setTextSize(1);
    gDisplay.setTextColor(SENSAIR_CYAN, SENSAIR_DARKGREY);
    gDisplay.setCursor(M + 6, y + 5);
    gDisplay.print(labels[i]);
    gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
    gDisplay.setCursor(M + 6, y + 17);
    gDisplay.print(values[i]);
  }
  button(M, 178, 100, 34, "BACK", SENSAIR_DARKGREY);
}

static void touchConn(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < 4; i++) {
    if (!inRect(x, y, M, CROW_Y0 + i * CROW_H, gDisplay.width() - 2 * M, CROW_H - 4)) {
      continue;
    }
    if (i == 0) {
      screen = SCR_WIFI;
      needRedraw = true;
    } else if (i == 1) {
      String url = Storage::serverUrl();
      if (RotaryInput::edit("Server URL", url, 200, App::backgroundTick)) {
        Storage::setServerUrl(url);
      }
      needRedraw = true;
    } else if (i == 2) {
      String id = Storage::deviceId();
      if (RotaryInput::edit("Device ID", id, 32, App::backgroundTick)) {
        Storage::setDeviceId(id);
      }
      needRedraw = true;
    } else {
      Storage::setServerUrl(APP_SERVER_URL_DEFAULT);
      LOGI("UI", "server URL reset to default");
      needRedraw = true;
    }
    return;
  }
  if (inRect(x, y, M, 178, 100, 34)) {
    screen = SCR_MAIN;
    needRedraw = true;
  }
}

/* ---------------- WIFI screen ---------------- */

static void scanNetworks() {
  title("WiFi networks");
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  gDisplay.setCursor(M, 60);
  gDisplay.print("scanning...");
  LOGI("UI", "wifi scan started");

  WiFi.mode(WIFI_STA);
  int16_t found = WiFi.scanNetworks();
  netCount = 0;
  for (uint8_t pick = 0; pick < MAX_NETS; pick++) {
    int16_t best = -1;
    for (int16_t i = 0; i < found; i++) {
      if (WiFi.SSID(i).length() == 0) continue;
      bool taken = false;
      for (uint8_t k = 0; k < netCount; k++) {
        if (netSsid[k] == WiFi.SSID(i)) taken = true;
      }
      if (taken) continue;
      if (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best)) best = i;
    }
    if (best < 0) break;
    netSsid[netCount] = WiFi.SSID(best);
    netRssi[netCount] = WiFi.RSSI(best);
    netOpen[netCount] = (WiFi.encryptionType(best) == WIFI_AUTH_OPEN);
    netCount++;
  }
  WiFi.scanDelete();
  LOGI("UI", "wifi scan: %d found, showing %u", found, netCount);
}

static void drawWifi() {
  scanNetworks();
  title("WiFi networks");
  if (netCount == 0) {
    gDisplay.setTextSize(2);
    gDisplay.setTextColor(SENSAIR_ORANGE, SENSAIR_BLACK);
    gDisplay.setCursor(M, 70);
    gDisplay.print("no networks found");
  }
  for (uint8_t i = 0; i < netCount; i++) {
    int16_t y = 30 + i * 26;
    gDisplay.fillRect(M, y, gDisplay.width() - 2 * M, 24, SENSAIR_DARKGREY);
    gDisplay.drawRect(M, y, gDisplay.width() - 2 * M, 24, SENSAIR_GREY);
    gDisplay.setTextSize(1);
    gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_DARKGREY);
    gDisplay.setCursor(M + 6, y + 8);
    gDisplay.print(netSsid[i].substring(0, 24));
    gDisplay.setTextColor(netOpen[i] ? SENSAIR_GREEN : SENSAIR_GREY, SENSAIR_DARKGREY);
    gDisplay.setCursor(gDisplay.width() - M - 56, y + 8);
    gDisplay.printf("%4ld %s", (long)netRssi[i], netOpen[i] ? "open" : "lock");
  }
  button(M, 194, 100, 34, "BACK", SENSAIR_DARKGREY);
  button(gDisplay.width() - M - 100, 194, 100, 34, "RESCAN", SENSAIR_NAVY);
}

static void connectFlow(uint8_t idx) {
  String pass;
  if (!netOpen[idx]) {
    String prompt = "Password for " + netSsid[idx].substring(0, 18);
    if (!RotaryInput::edit(prompt.c_str(), pass, 63, App::backgroundTick)) {
      needRedraw = true;
      return;
    }
  }
  title("WiFi");
  gDisplay.setTextSize(2);
  gDisplay.setTextColor(SENSAIR_WHITE, SENSAIR_BLACK);
  gDisplay.setCursor(M, 60);
  gDisplay.printf("connecting to\n");
  gDisplay.setCursor(M, 84);
  gDisplay.print(netSsid[idx].substring(0, 21));

  bool ok = AlarmNet::connectTo(netSsid[idx], pass, APP_WIFI_TIMEOUT_MS);
  gDisplay.setCursor(M, 120);
  if (ok) {
    Storage::setWifi(netSsid[idx], pass);
    gDisplay.setTextColor(SENSAIR_GREEN, SENSAIR_BLACK);
    gDisplay.printf("connected, %s", AlarmNet::ip().c_str());
    screen = SCR_CONN;
  } else {
    gDisplay.setTextColor(SENSAIR_RED, SENSAIR_BLACK);
    gDisplay.print("failed (password?)");
  }
  delay(1500);
  needRedraw = true;
}

static void touchWifi(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < netCount; i++) {
    if (inRect(x, y, M, 30 + i * 26, gDisplay.width() - 2 * M, 24)) {
      connectFlow(i);
      return;
    }
  }
  if (inRect(x, y, M, 194, 100, 34)) {
    screen = SCR_CONN;
    needRedraw = true;
  } else if (inRect(x, y, gDisplay.width() - M - 100, 194, 100, 34)) {
    needRedraw = true;  /* rescan = redraw */
  }
}

/* ---------------- dispatcher ---------------- */

void begin() {
  needRedraw = true;
  LOGI("UI", "ready");
}

void tick(uint32_t now) {
  /* a sounding local alarm always pulls the UI to the STOP screen */
  if (AlarmManager::anyLocalActive() && screen != SCR_MAIN) {
    LOGI("UI", "local alarm active -> jumping to main screen");
    screen = SCR_MAIN;
    needRedraw = true;
  }

  if (needRedraw) {
    needRedraw = false;
    switch (screen) {
      case SCR_MAIN:   drawMain(); break;
      case SCR_ALARMS: drawAlarmList(); break;
      case SCR_EDIT:   drawEdit(); break;
      case SCR_CONN:   drawConn(); break;
      case SCR_WIFI:   drawWifi(); break;
    }
    nextRefresh = now + APP_UI_REFRESH_MS;
  }

  /* periodic refresh of the live parts of the main screen */
  if (screen == SCR_MAIN && (int32_t)(now - nextRefresh) >= 0) {
    nextRefresh = now + APP_UI_REFRESH_MS;
    drawMainValues();
    bool active = AlarmManager::anyLocalActive();
    if (active && !stopShown) {
      drawStopOverlay();
    } else if (!active && stopShown) {
      needRedraw = true;  /* alarm ended elsewhere: restore buttons */
    } else if (!active) {
      drawStatusLine();
    }
  }

  /* touch handling: act on new contact; +/- buttons auto-repeat */
  static bool contact = false;
  SensairTouchPoint p;
  bool down = gTouch.read(p) && p.touched;

  if (down && !contact) {
    heldZone = 0;
    switch (screen) {
      case SCR_MAIN:   touchMain(p.x, p.y); break;
      case SCR_ALARMS: touchAlarmList(p.x, p.y); break;
      case SCR_EDIT:   touchEdit(p.x, p.y); break;
      case SCR_CONN:   touchConn(p.x, p.y); break;
      case SCR_WIFI:   touchWifi(p.x, p.y); break;
    }
  } else if (down && screen == SCR_EDIT && heldZone &&
             (int32_t)(now - nextRepeat) >= 0) {
    editStep(heldZone);
    nextRepeat = now + 120;
  } else if (!down) {
    heldZone = 0;
  }
  contact = down;
}

}  // namespace Ui
