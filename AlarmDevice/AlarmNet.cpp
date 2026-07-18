/*
 * AlarmNet.cpp — WiFi + webhook delivery for the Alarm Device.
 */
#include "AlarmNet.h"
#include "AppConfig.h"
#include "DebugLog.h"
#include "Storage.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

namespace AlarmNet {

static uint32_t nextReconnect = 0;
static uint32_t nextRetry = 0;

void begin() {
  WiFi.mode(WIFI_STA);
  String ssid = Storage::wifiSsid();
  if (ssid.length()) {
    LOGI("NET", "starting connection to stored network '%s'", ssid.c_str());
    WiFi.begin(ssid.c_str(), Storage::wifiPass().c_str());
  } else {
    LOGI("NET", "no stored WiFi credentials");
  }
  if (Storage::pendingCount()) {
    LOGW("NET", "%u unsent alarm(s) recovered from NVM, will retry",
         Storage::pendingCount());
  }
}

bool connected() { return WiFi.status() == WL_CONNECTED; }
String ip() { return connected() ? WiFi.localIP().toString() : String("-"); }
int8_t rssi() { return connected() ? (int8_t)WiFi.RSSI() : 0; }

bool connectTo(const String &ssid, const String &pass, uint32_t timeoutMs) {
  LOGI("NET", "connecting to '%s'...", ssid.c_str());
  WiFi.disconnect();
  delay(100);
  if (pass.length()) WiFi.begin(ssid.c_str(), pass.c_str());
  else WiFi.begin(ssid.c_str());

  uint32_t deadline = millis() + timeoutMs;
  while (WiFi.status() != WL_CONNECTED && (int32_t)(millis() - deadline) < 0) {
    delay(100);
  }
  bool ok = WiFi.status() == WL_CONNECTED;
  LOGI("NET", "connect %s (ip %s)", ok ? "OK" : "FAILED", ip().c_str());
  return ok;
}

String urlEncode(const String &s) {
  String out;
  out.reserve(s.length() * 3);
  const char *hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

/* One delivery attempt. Returns true when the server accepted it. */
static bool sendNow(const String &text) {
  if (!connected()) {          /* verify WiFi before sending */
    LOGW("NET", "send skipped: WiFi not connected");
    return false;
  }
  String url = Storage::serverUrl();
  if (!url.length()) {
    LOGE("NET", "send skipped: no server URL configured");
    return false;
  }
  url += "?value1=" + urlEncode(text) + "&value2=" + urlEncode(Storage::deviceId());
  LOGI("NET", "GET %s", url.c_str());

  HTTPClient http;
  http.setTimeout(APP_HTTP_TIMEOUT_MS);
  http.setConnectTimeout(APP_HTTP_TIMEOUT_MS);
  bool began;
  WiFiClientSecure secureClient;   /* scoped: freed after the request */
  WiFiClient plainClient;
  if (url.startsWith("https")) {
    secureClient.setInsecure();    /* no certificate validation */
    began = http.begin(secureClient, url);
  } else {
    began = http.begin(plainClient, url);
  }
  if (!began) {
    LOGE("NET", "http.begin failed");
    return false;
  }
  int code = http.GET();
  http.end();
  LOGI("NET", "HTTP result: %d", code);
  return code >= 200 && code < 300;
}

/* Try to deliver everything in the queue (oldest first). */
static void flushQueue() {
  while (Storage::pendingCount()) {
    String text = Storage::pendingAt(0);
    if (!sendNow(text)) {
      LOGW("NET", "delivery failed, %u alarm(s) stay queued (retry in %d min)",
           Storage::pendingCount(), APP_RETRY_MINUTES);
      return;
    }
    LOGI("NET", "delivered: \"%s\"", text.c_str());
    Storage::removePending(0);
  }
}

void queueAlarm(const String &text) {
  Storage::pushPending(text);   /* persist FIRST — survives power loss */
  flushQueue();
  nextRetry = millis() + (uint32_t)APP_RETRY_MINUTES * 60000UL;
}

uint8_t pendingCount() { return Storage::pendingCount(); }

void tick(uint32_t now) {
  /* auto-reconnect with stored credentials */
  if (!connected() && Storage::wifiSsid().length() &&
      (int32_t)(now - nextReconnect) >= 0) {
    nextReconnect = now + (uint32_t)APP_RECONNECT_S * 1000UL;
    LOGI("NET", "reconnect attempt to '%s'", Storage::wifiSsid().c_str());
    WiFi.disconnect();
    WiFi.begin(Storage::wifiSsid().c_str(), Storage::wifiPass().c_str());
  }

  /* periodic retry of unsent alarms */
  if (Storage::pendingCount() && (int32_t)(now - nextRetry) >= 0) {
    nextRetry = now + (uint32_t)APP_RETRY_MINUTES * 60000UL;
    LOGI("NET", "retrying %u unsent alarm(s)", Storage::pendingCount());
    flushQueue();
  }
}

}  // namespace AlarmNet
