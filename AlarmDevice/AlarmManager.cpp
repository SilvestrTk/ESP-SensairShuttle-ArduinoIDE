/*
 * AlarmManager.cpp — alarm evaluation for the Alarm Device.
 */
#include "AlarmManager.h"
#include "AppConfig.h"
#include "DebugLog.h"
#include "Storage.h"
#include "Sensors.h"
#include "AlarmSiren.h"
#include "AlarmNet.h"

/* --------------------------------------------------------------------------
 * The alarm definition table. TO ADD A NEW ALARM: add an id in
 * AlarmTypes.h, a row here, and its measurement in Sensors::valueFor().
 * ------------------------------------------------------------------------*/
const AlarmDef ALARM_DEFS[ALARM_COUNT] = {
  /* id           name           short        unit   min   max    step  default               dec */
  {ALARM_TEMP,  "Temperature", "Temp",      "C",   -20,  80,    0.5,  APP_DEF_TEMP_THRESH,  1},
  {ALARM_HUM,   "Humidity",    "Humidity",  "%",   5,    100,   1,    APP_DEF_HUM_THRESH,   0},
  {ALARM_AIR,   "Air quality", "Air (IAQ)", "IAQ", 50,   500,   10,   APP_DEF_AIR_THRESH,   0},
  {ALARM_VIB,   "Vibration",   "Vibration", "mg",  10,   2000,  10,   APP_DEF_VIB_THRESH,   0},
  {ALARM_SOUND, "Sound level", "Sound",     "mV",  10,   1000,  10,   APP_DEF_SOUND_THRESH, 0},
};

namespace AlarmManager {

static AlarmRuntime rt[ALARM_COUNT];

void begin() {
  LOGI("ALRM", "%u alarm types registered", ALARM_COUNT);
}

void evaluate(const AlarmSettings &s, float value, bool valid, uint32_t now,
              AlarmRuntime &r, bool &fireLocal, bool &fireRemote) {
  fireLocal = false;
  fireRemote = false;
  r.lastValue = value;
  r.valid = valid;

  bool over = valid && value > s.threshold;
  r.overThreshold = over;

  if (s.localEnabled) {
    if (!over) r.localAcked = false;  /* condition cleared: re-arm */
    if (over && !r.localActive && !r.localAcked) {
      r.localActive = true;
      fireLocal = true;
    }
  } else {
    r.localActive = false;
    r.localAcked = false;
  }

  if (s.remoteEnabled && over &&
      (r.remoteReadyAt == 0 || (int32_t)(now - r.remoteReadyAt) >= 0)) {
    fireRemote = true;
    r.remoteReadyAt = now + (uint32_t)s.resetMinutes * 60000UL;
    if (r.remoteReadyAt == 0) r.remoteReadyAt = 1;  /* keep 0 = "never fired" */
  }
}

void update(uint32_t now) {
  bool anyLocal = false;

  for (uint8_t i = 0; i < ALARM_COUNT; i++) {
    const AlarmSettings &s = Storage::alarmCfg(i);
    if (!s.localEnabled && !s.remoteEnabled) {
      rt[i].localActive = false;
      continue;
    }
    bool valid = false;
    float value = Sensors::valueFor(i, valid);

    bool fireLocal = false, fireRemote = false;
    evaluate(s, value, valid, now, rt[i], fireLocal, fireRemote);

    if (fireLocal) {
      LOGW("ALRM", "LOCAL alarm: %s = %.1f %s > %.1f", ALARM_DEFS[i].name,
           value, ALARM_DEFS[i].unit, s.threshold);
    }
    if (fireRemote) {
      String text = payloadText(i, s.threshold, value);
      LOGW("ALRM", "REMOTE alarm: %s (re-arms in %u min)", text.c_str(),
           s.resetMinutes);
      AlarmNet::queueAlarm(text);
    }
    if (rt[i].localActive) anyLocal = true;
  }

  AlarmSiren::setActive(anyLocal);
}

bool anyLocalActive() {
  for (uint8_t i = 0; i < ALARM_COUNT; i++) {
    if (rt[i].localActive) return true;
  }
  return false;
}

void stopAllLocal() {
  LOGW("ALRM", "STOP ALARM pressed — silencing local alarms");
  for (uint8_t i = 0; i < ALARM_COUNT; i++) {
    if (rt[i].localActive || rt[i].overThreshold) {
      rt[i].localAcked = true;  /* no re-trigger until value drops */
    }
    rt[i].localActive = false;
  }
  AlarmSiren::setActive(false);
}

const AlarmRuntime &runtime(uint8_t id) {
  return rt[id < ALARM_COUNT ? id : 0];
}

String payloadText(uint8_t id, float threshold, float value) {
  const AlarmDef &d = ALARM_DEFS[id < ALARM_COUNT ? id : 0];
  char buf[120];
  snprintf(buf, sizeof(buf), "%s exceeded threshold %.*f %s (measured %.*f %s)",
           d.name, d.decimals, threshold, d.unit, d.decimals, value, d.unit);
  return String(buf);
}

}  // namespace AlarmManager
