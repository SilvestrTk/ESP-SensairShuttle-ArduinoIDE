/*
 * Tests.cpp — self-test suite of the Alarm Device.
 * Covers the pure logic (alarm evaluation, URL encoding, IAQ math,
 * payload formatting) and the NVM layer (settings + pending queue).
 */
#include "Tests.h"
#include "AppConfig.h"
#include "AlarmTypes.h"
#include "AlarmManager.h"
#include "AlarmNet.h"
#include "Sensors.h"
#include "Storage.h"
#include <Arduino.h>

namespace Tests {

static int passed = 0, failed = 0;

static void check(const char *name, bool ok) {
  if (ok) {
    passed++;
    Serial.printf("PASS  %s\n", name);
  } else {
    failed++;
    Serial.printf("FAIL  %s\n", name);
  }
}

/* ---------------- URL encoding ---------------- */
static void testUrlEncode() {
  check("urlencode: plain text unchanged",
        AlarmNet::urlEncode("Sensor-001_x.y~z") == "Sensor-001_x.y~z");
  check("urlencode: space and symbols",
        AlarmNet::urlEncode("a b&c=d") == "a%20b%26c%3Dd");
  check("urlencode: slash and percent",
        AlarmNet::urlEncode("/100%") == "%2F100%25");
}

/* ---------------- payload text ---------------- */
static void testPayload() {
  String t = AlarmManager::payloadText(ALARM_TEMP, 35.0f, 36.2f);
  check("payload: contains alarm name", t.indexOf("Temperature") == 0);
  check("payload: contains threshold", t.indexOf("35.0") > 0);
  check("payload: contains measured value", t.indexOf("36.2") > 0);
}

/* ---------------- alarm evaluation ---------------- */
static void testEvaluate() {
  AlarmSettings s{30.0f, true, true, 5};
  AlarmRuntime r;
  bool fl, fr;

  /* below threshold: nothing fires */
  AlarmManager::evaluate(s, 25.0f, true, 1000, r, fl, fr);
  check("eval: below threshold, no local", !fl && !r.localActive);
  check("eval: below threshold, no remote", !fr);

  /* crossing: both fire, local latches */
  AlarmManager::evaluate(s, 31.0f, true, 2000, r, fl, fr);
  check("eval: crossing fires local", fl && r.localActive);
  check("eval: crossing fires remote", fr);

  /* still over: no re-fire (local latched, remote in reset period) */
  AlarmManager::evaluate(s, 32.0f, true, 3000, r, fl, fr);
  check("eval: latched local does not re-fire", !fl && r.localActive);
  check("eval: remote respects reset period", !fr);

  /* user stop: acked, still over threshold -> stays silent */
  r.localActive = false;
  r.localAcked = true;
  AlarmManager::evaluate(s, 32.0f, true, 4000, r, fl, fr);
  check("eval: acked alarm stays silent while over", !fl && !r.localActive);

  /* value drops, then rises again -> local re-fires */
  AlarmManager::evaluate(s, 20.0f, true, 5000, r, fl, fr);
  AlarmManager::evaluate(s, 33.0f, true, 6000, r, fl, fr);
  check("eval: local re-fires after value dropped", fl && r.localActive);

  /* remote re-fires only after the reset period (5 min) */
  AlarmManager::evaluate(s, 33.0f, true, 2000 + 5 * 60000UL - 1000, r, fl, fr);
  check("eval: remote still quiet before reset time", !fr);
  AlarmManager::evaluate(s, 33.0f, true, 2000 + 5 * 60000UL + 1000, r, fl, fr);
  check("eval: remote re-fires after reset time", fr);

  /* invalid readings never trigger */
  AlarmRuntime r2;
  AlarmManager::evaluate(s, 99.0f, false, 1000, r2, fl, fr);
  check("eval: invalid reading never triggers", !fl && !fr && !r2.localActive);

  /* disabled alarm never triggers */
  AlarmSettings off{30.0f, false, false, 5};
  AlarmRuntime r3;
  AlarmManager::evaluate(off, 99.0f, true, 1000, r3, fl, fr);
  check("eval: disabled alarm never triggers", !fl && !fr);
}

/* ---------------- IAQ approximation ---------------- */
static void testIaq() {
  float clean = Sensors::computeIaq(100000, 100000, 40.0f);
  check("iaq: clean air scores ~0", clean >= 0 && clean <= 25);
  float dirty = Sensors::computeIaq(10000, 100000, 40.0f);
  check("iaq: low gas resistance scores worse", dirty > clean + 100);
  float worst = Sensors::computeIaq(1000, 100000, 90.0f);
  check("iaq: bounded by 500", worst > 300 && worst <= 500);
}

/* ---------------- NVM storage ---------------- */
static void testStorage() {
  Storage::beginTestNamespace();  /* scratch namespace, wiped */

  AlarmSettings &a = Storage::alarmCfg(ALARM_TEMP);
  check("storage: default threshold applied",
        fabsf(a.threshold - ALARM_DEFS[ALARM_TEMP].defThreshold) < 0.01f);
  check("storage: alarms default to off", !a.localEnabled && !a.remoteEnabled);

  a.threshold = 42.5f;
  a.localEnabled = true;
  a.resetMinutes = 7;
  Storage::saveAlarm(ALARM_TEMP);

  Storage::setWifi("TestNet", "secret123");
  Storage::setDeviceId("Unit test 7");
  check("storage: wifi roundtrip",
        Storage::wifiSsid() == "TestNet" && Storage::wifiPass() == "secret123");
  check("storage: device id roundtrip", Storage::deviceId() == "Unit test 7");
  check("storage: default server url present",
        Storage::serverUrl().startsWith("https://maker.ifttt.com"));

  /* pending queue: FIFO with drop-oldest */
  check("storage: queue starts empty", Storage::pendingCount() == 0);
  Storage::pushPending("alarm A");
  Storage::pushPending("alarm B");
  check("storage: queue count", Storage::pendingCount() == 2);
  check("storage: queue order", Storage::pendingAt(0) == "alarm A");
  Storage::removePending(0);
  check("storage: queue shift after remove",
        Storage::pendingCount() == 1 && Storage::pendingAt(0) == "alarm B");
  for (uint8_t i = 0; i < APP_PENDING_MAX + 2; i++) {
    Storage::pushPending("bulk " + String(i));
  }
  check("storage: queue capacity respected",
        Storage::pendingCount() == APP_PENDING_MAX);
  while (Storage::pendingCount()) Storage::removePending(0);

  Storage::begin();  /* back to the real namespace */
}

void run() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("=== Alarm Device self-test suite ===");
  Serial.println();

  testUrlEncode();
  testPayload();
  testEvaluate();
  testIaq();
  testStorage();

  Serial.println();
  Serial.printf("=== result: %d passed, %d failed — %s ===\n", passed, failed,
                failed == 0 ? "ALL TESTS PASSED" : "TESTS FAILED");
}

}  // namespace Tests
