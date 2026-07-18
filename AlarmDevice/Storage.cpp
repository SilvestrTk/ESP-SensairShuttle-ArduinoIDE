/*
 * Storage.cpp — NVM persistence for the Alarm Device.
 */
#include "Storage.h"
#include "AppConfig.h"
#include "DebugLog.h"
#include <Preferences.h>

namespace Storage {

static Preferences prefs;
static AlarmSettings alarms[ALARM_COUNT];
static const uint8_t LAYOUT_VERSION = 1;

static void alarmKey(uint8_t id, char *buf) {
  snprintf(buf, 8, "al%u", id);
}

static void loadAlarms() {
  for (uint8_t i = 0; i < ALARM_COUNT; i++) {
    char key[8];
    alarmKey(i, key);
    if (prefs.getBytesLength(key) == sizeof(AlarmSettings)) {
      prefs.getBytes(key, &alarms[i], sizeof(AlarmSettings));
    } else {
      /* defaults: alarm OFF, threshold from the definition table */
      alarms[i].threshold = ALARM_DEFS[i].defThreshold;
      alarms[i].localEnabled = false;
      alarms[i].remoteEnabled = false;
      alarms[i].resetMinutes = APP_DEF_RESET_MIN;
    }
    LOGI("STOR", "alarm %u: thr=%.1f local=%d remote=%d reset=%umin",
         i, alarms[i].threshold, alarms[i].localEnabled,
         alarms[i].remoteEnabled, alarms[i].resetMinutes);
  }
}

static void open(const char *ns) {
  prefs.end();
  prefs.begin(ns, false);
  if (prefs.getUChar("ver", 0) != LAYOUT_VERSION) {
    LOGW("STOR", "namespace '%s': new or outdated layout, initializing", ns);
    prefs.clear();
    prefs.putUChar("ver", LAYOUT_VERSION);
  }
  loadAlarms();
}

void begin() {
  open("alarmdev");
  LOGI("STOR", "loaded: id='%s' ssid='%s' url set=%d pending=%u",
       deviceId().c_str(), wifiSsid().c_str(),
       serverUrl().length() > 0, pendingCount());
}

void beginTestNamespace() {
  open("alarmtest");
  prefs.clear();
  prefs.putUChar("ver", LAYOUT_VERSION);
  loadAlarms();
}

AlarmSettings &alarmCfg(uint8_t id) {
  return alarms[id < ALARM_COUNT ? id : 0];
}

void saveAlarm(uint8_t id) {
  if (id >= ALARM_COUNT) return;
  char key[8];
  alarmKey(id, key);
  prefs.putBytes(key, &alarms[id], sizeof(AlarmSettings));
  LOGI("STOR", "alarm %u saved", id);
}

String wifiSsid() { return prefs.getString("wssid", ""); }
String wifiPass() { return prefs.getString("wpass", ""); }

void setWifi(const String &ssid, const String &pass) {
  prefs.putString("wssid", ssid);
  prefs.putString("wpass", pass);
  LOGI("STOR", "wifi credentials saved for '%s'", ssid.c_str());
}

String serverUrl() { return prefs.getString("url", APP_SERVER_URL_DEFAULT); }

void setServerUrl(const String &url) {
  prefs.putString("url", url);
  LOGI("STOR", "server url saved (%u chars)", url.length());
}

String deviceId() { return prefs.getString("devid", APP_DEVICE_ID_DEFAULT); }

void setDeviceId(const String &id) {
  prefs.putString("devid", id);
  LOGI("STOR", "device id saved: '%s'", id.c_str());
}

/* ---------------- unsent alarm queue ---------------- */

static void pendingKey(uint8_t i, char *buf) {
  snprintf(buf, 8, "pq%u", i);
}

uint8_t pendingCount() {
  return prefs.getUChar("pqn", 0);
}

String pendingAt(uint8_t i) {
  if (i >= pendingCount()) return String();
  char key[8];
  pendingKey(i, key);
  return prefs.getString(key, "");
}

void pushPending(const String &text) {
  uint8_t n = pendingCount();
  if (n >= APP_PENDING_MAX) {
    LOGW("STOR", "pending queue full, dropping oldest");
    removePending(0);
    n = pendingCount();
  }
  char key[8];
  pendingKey(n, key);
  prefs.putString(key, text);
  prefs.putUChar("pqn", n + 1);
  LOGI("STOR", "pending[%u] <- \"%s\"", n, text.c_str());
}

void removePending(uint8_t i) {
  uint8_t n = pendingCount();
  if (i >= n) return;
  char key[8];
  /* shift the rest down */
  for (uint8_t k = i; k + 1 < n; k++) {
    char next[8];
    pendingKey(k + 1, next);
    String v = prefs.getString(next, "");
    pendingKey(k, key);
    prefs.putString(key, v);
  }
  pendingKey(n - 1, key);
  prefs.remove(key);
  prefs.putUChar("pqn", n - 1);
}

}  // namespace Storage
