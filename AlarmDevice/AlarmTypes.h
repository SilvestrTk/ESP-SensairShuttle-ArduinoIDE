/*
 * AlarmTypes.h — shared data types of the Alarm Device.
 *
 * TO ADD A NEW ALARM TYPE:
 *   1. add an id before ALARM_COUNT below
 *   2. add a row to ALARM_DEFS[] in AlarmManager.cpp
 *   3. provide its measurement in Sensors::valueFor()
 * Storage, evaluation, UI list/editor, siren and remote reporting pick
 * the new alarm up automatically.
 */
#ifndef ALARM_TYPES_H
#define ALARM_TYPES_H

#include <Arduino.h>

enum : uint8_t {
  ALARM_TEMP = 0,
  ALARM_HUM,
  ALARM_AIR,
  ALARM_VIB,
  ALARM_SOUND,
  ALARM_COUNT
};

/* Per-alarm user settings, persisted in NVM. */
struct AlarmSettings {
  float threshold;
  bool localEnabled;      /* sound the siren             */
  bool remoteEnabled;     /* send webhook to the server  */
  uint16_t resetMinutes;  /* remote re-arm period        */
};

/* Per-alarm runtime state (not persisted). */
struct AlarmRuntime {
  bool localActive = false;    /* siren latched on, until STOP ALARM  */
  bool localAcked = false;     /* stopped by user; re-arms when the
                                  value drops below the threshold     */
  uint32_t remoteReadyAt = 0;  /* millis() when remote may fire again */
  bool overThreshold = false;
  float lastValue = 0;
  bool valid = false;
};

/* Static description of one alarm type. */
struct AlarmDef {
  uint8_t id;
  const char *name;       /* full name, used in the payload text  */
  const char *shortName;  /* fits the main-screen row             */
  const char *unit;
  float min, max, step;   /* threshold editor range               */
  float defThreshold;
  uint8_t decimals;       /* display precision                    */
};

/* Latest measurements, produced by the Sensors module. */
struct SensorReadings {
  float temperature = 0, humidity = 0, pressure = 0;
  bool envValid = false;
  float iaq = 0;           /* 0..500, higher = worse (approximation) */
  bool iaqValid = false;
  float vibrationMg = 0;   /* std-dev of |accel| over the window     */
  bool vibValid = false;
  float soundMv = 0;       /* microphone RMS                         */
  bool soundValid = false;
};

extern const AlarmDef ALARM_DEFS[ALARM_COUNT];

#endif /* ALARM_TYPES_H */
