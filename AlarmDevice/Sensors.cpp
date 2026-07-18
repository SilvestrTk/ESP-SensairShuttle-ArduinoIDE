/*
 * Sensors.cpp — measurement scheduler of the Alarm Device.
 */
#include "Sensors.h"
#include "AppConfig.h"
#include "DebugLog.h"
#include <SensairBME690.h>
#include <SensairBMI270.h>
#include <SensairMic.h>
#include <math.h>

namespace Sensors {

static SensairBME690 bme;
static SensairBMI270 imu;
static SensairMic mic;

static bool envPresent = false;
static bool imuPresent = false;
static bool micPresent = false;

static SensorReadings data;

/* schedulers */
static uint32_t nextEnv = 0;
static uint32_t nextSound = 0;
static uint32_t nextVibSample = 0;
static uint32_t vibWindowEnd = 0;
static uint32_t bootMillis = 0;

/* vibration accumulator (|accel| magnitude in g) */
static uint32_t vibN = 0;
static double vibSum = 0;
static double vibSumSq = 0;

/* IAQ baseline: adaptive maximum of the gas resistance */
static float gasBaseline = 0;

void begin() {
  bootMillis = millis();

  envPresent = bme.begin();
  LOGI("SENS", "BME690 (env): %s", envPresent ? "present" : "absent");

  if (!envPresent) {
    imuPresent = imu.begin();
  } else {
    /* one shuttle connector — motion board can't be there too, but
     * probe anyway in case future hardware allows both */
    imuPresent = imu.begin();
  }
  LOGI("SENS", "BMI270 (vibration): %s", imuPresent ? "present" : "absent");

  micPresent = mic.begin();
  LOGI("SENS", "microphone: %s", micPresent ? "present" : "absent");
}

static void readEnv(uint32_t now) {
  SensairEnvData env;
  if (!bme.read(env)) {
    LOGW("SENS", "BME690 read failed (rc %d)", bme.lastResult());
    data.envValid = false;
    data.iaqValid = false;
    return;
  }
  data.temperature = env.temperature;
  data.humidity = env.humidity;
  data.pressure = env.pressure;
  data.envValid = true;

  /* IAQ from gas resistance + humidity */
  if (env.gasValid && env.heaterStable && env.gasResistance > 0) {
    if (env.gasResistance > gasBaseline) {
      gasBaseline = env.gasResistance;         /* rise instantly     */
    } else {
      gasBaseline *= 0.9995f;                  /* decay very slowly  */
    }
    data.iaq = computeIaq(env.gasResistance, gasBaseline, env.humidity);
    data.iaqValid = (now - bootMillis) >= APP_IAQ_WARMUP_MS;
  }
  LOGV("SENS", "env: %.2fC %.1f%% %.1fhPa gas=%.0f base=%.0f iaq=%.0f%s",
       data.temperature, data.humidity, data.pressure,
       env.gasResistance, gasBaseline, data.iaq,
       data.iaqValid ? "" : " (warming up)");
}

static void sampleVibration() {
  SensairMotionData m;
  if (!imu.read(m)) return;
  double mag = sqrt((double)m.ax * m.ax + (double)m.ay * m.ay + (double)m.az * m.az);
  vibSum += mag;
  vibSumSq += mag * mag;
  vibN++;
}

static void finishVibrationWindow() {
  if (vibN >= 10) {
    double mean = vibSum / vibN;
    double var = vibSumSq / vibN - mean * mean;
    data.vibrationMg = var > 0 ? (float)(sqrt(var) * 1000.0) : 0.0f;
    data.vibValid = true;
    LOGV("SENS", "vibration: %.1f mg (%lu samples)", data.vibrationMg,
         (unsigned long)vibN);
  }
  vibN = 0;
  vibSum = 0;
  vibSumSq = 0;
}

void tick(uint32_t now) {
  if (envPresent && (int32_t)(now - nextEnv) >= 0) {
    nextEnv = now + APP_ENV_PERIOD_MS;
    readEnv(now);
  }
  if (imuPresent) {
    if ((int32_t)(now - nextVibSample) >= 0) {
      nextVibSample = now + APP_VIB_SAMPLE_MS;
      sampleVibration();
    }
    if ((int32_t)(now - vibWindowEnd) >= 0) {
      vibWindowEnd = now + APP_VIB_WINDOW_MS;
      finishVibrationWindow();
    }
  }
  if (micPresent && (int32_t)(now - nextSound) >= 0) {
    nextSound = now + APP_SOUND_PERIOD_MS;
    SensairSoundLevel lv = mic.readLevel(APP_SOUND_WINDOW_MS);
    data.soundMv = lv.rmsMv;
    data.soundValid = lv.samples > 0;
  }
}

const SensorReadings &readings() { return data; }
bool hasEnv() { return envPresent; }
bool hasImu() { return imuPresent; }
bool hasMic() { return micPresent; }

float valueFor(uint8_t alarmId, bool &valid) {
  switch (alarmId) {
    case ALARM_TEMP:  valid = data.envValid;   return data.temperature;
    case ALARM_HUM:   valid = data.envValid;   return data.humidity;
    case ALARM_AIR:   valid = data.iaqValid;   return data.iaq;
    case ALARM_VIB:   valid = data.vibValid;   return data.vibrationMg;
    case ALARM_SOUND: valid = data.soundValid; return data.soundMv;
    default:          valid = false;           return 0;
  }
}

float computeIaq(float gasOhm, float gasBaselineOhm, float humidityPct) {
  /* humidity contribution, 25%: best at 40 %RH */
  float humScore;
  float dh = fabsf(humidityPct - 40.0f);
  humScore = 25.0f * (1.0f - fminf(dh / 40.0f, 1.0f));

  /* gas contribution, 75%: current resistance vs. the clean-air baseline */
  float gasScore = 0;
  if (gasBaselineOhm > 0) {
    gasScore = 75.0f * fminf(gasOhm / gasBaselineOhm, 1.0f);
  }

  float score = humScore + gasScore;      /* 0..100, higher = better */
  return (100.0f - score) * 5.0f;         /* 0..500,  higher = worse */
}

const char *iaqLevelName(float iaq) {
  if (iaq <= 50)  return "Excellent";
  if (iaq <= 100) return "Good";
  if (iaq <= 150) return "Fair";
  if (iaq <= 200) return "Poor";
  if (iaq <= 300) return "Bad";
  return "Very Bad";
}

}  // namespace Sensors
