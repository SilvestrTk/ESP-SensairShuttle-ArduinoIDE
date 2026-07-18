/*
 * Sensors.h — measurement scheduler of the Alarm Device.
 *
 * Reads whatever hardware is present (auto-detected at boot):
 *   - BME690 shuttle board  -> temperature, humidity, pressure, IAQ
 *   - BMI270 shuttle board  -> vibration (std-dev of |accel| over a window)
 *   - on-board microphone   -> sound level (RMS)
 *
 * NOTE: the board has a single shuttle connector, so the environmental
 * and the motion board cannot be inserted at the same time — readings
 * from the missing board are flagged invalid and their alarms are shown
 * as "n/a" and skipped.
 *
 * IAQ: Bosch's factory-grade IAQ needs the proprietary BSEC2 library;
 * this module computes the common approximation (gas resistance vs. an
 * adaptive baseline + humidity deviation) mapped to the factory 0..500
 * scale (higher = worse) with the same level names.
 */
#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "AlarmTypes.h"

namespace Sensors {

void begin();
void tick(uint32_t now);                 /* call every loop */
const SensorReadings &readings();

bool hasEnv();
bool hasImu();
bool hasMic();

/* Measurement for one alarm id; valid=false when hardware is absent
 * or the value is not ready yet (e.g. IAQ warm-up). */
float valueFor(uint8_t alarmId, bool &valid);

const char *iaqLevelName(float iaq);

/* Pure IAQ computation, exposed for the test suite. */
float computeIaq(float gasOhm, float gasBaselineOhm, float humidityPct);

}  // namespace Sensors

#endif /* SENSORS_H */
