/*
 * Storage.h — NVM persistence (ESP32 NVS via Preferences).
 *
 * Persists: alarm settings, WiFi credentials, server URL, device ID and
 * the queue of unsent alarms — everything survives power loss / reset.
 */
#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "AlarmTypes.h"

namespace Storage {

/* Load everything from NVM (or defaults on first boot). */
void begin();

/* --- alarm settings --- */
AlarmSettings &alarmCfg(uint8_t id);   /* live reference        */
void saveAlarm(uint8_t id);            /* persist one alarm     */

/* --- connection settings --- */
String wifiSsid();
String wifiPass();
void setWifi(const String &ssid, const String &pass);
String serverUrl();
void setServerUrl(const String &url);
String deviceId();
void setDeviceId(const String &id);

/* --- unsent alarm queue (FIFO, capacity APP_PENDING_MAX) --- */
uint8_t pendingCount();
String pendingAt(uint8_t i);
void pushPending(const String &text);  /* drops the oldest when full */
void removePending(uint8_t i);

/* Test support: switch to a scratch NVS namespace. */
void beginTestNamespace();

}  // namespace Storage

#endif /* STORAGE_H */
