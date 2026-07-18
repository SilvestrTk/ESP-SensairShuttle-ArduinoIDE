/*
 * AlarmNet.h — WiFi connection + webhook delivery with a persistent
 * retry queue.
 *
 * Delivery contract:
 *   - queueAlarm() ALWAYS stores the alarm in NVM first (survives power
 *     loss), then attempts delivery immediately.
 *   - WiFi connectivity is verified before every send.
 *   - Failed sends stay in the queue; tick() retries them every
 *     APP_RETRY_MINUTES. After a reboot the queue is reloaded from NVM
 *     and retried.
 *
 * Payload: GET <serverUrl>?value1=<alarm text>&value2=<device id>,
 * both URL-encoded. HTTPS is supported (certificate not validated).
 */
#ifndef ALARM_NET_H
#define ALARM_NET_H

#include <Arduino.h>

namespace AlarmNet {

void begin();
void tick(uint32_t now);             /* reconnect + retry pending */

bool connected();
String ip();
int8_t rssi();

/* Blocking connect used by the UI (also fires WiFi.begin with stored
 * credentials at boot). Does not persist anything. */
bool connectTo(const String &ssid, const String &pass, uint32_t timeoutMs);

/* Store the alarm and try to deliver it now. */
void queueAlarm(const String &text);

/* Number of alarms waiting for delivery. */
uint8_t pendingCount();

String urlEncode(const String &s);   /* exposed for the test suite */

}  // namespace AlarmNet

#endif /* ALARM_NET_H */
