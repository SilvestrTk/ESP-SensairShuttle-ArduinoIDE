/*
 * AlarmManager.h — evaluates all alarms against the current readings.
 *
 * Semantics (per alarm, independent — alarms run in parallel):
 *   LOCAL:  latches the siren ON when the value crosses the threshold.
 *           Stays on until the user presses STOP ALARM. After a stop it
 *           re-arms only once the value has dropped below the threshold
 *           (no immediate re-trigger while still over it).
 *   REMOTE: sends the webhook when the value crosses the threshold, then
 *           won't send again until the user-defined reset period passed.
 *
 * The core evaluation is a pure function (evaluate) so the test suite
 * can exercise it without hardware.
 */
#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>
#include "AlarmTypes.h"

namespace AlarmManager {

void begin();
void update(uint32_t now);            /* call every loop */

bool anyLocalActive();
void stopAllLocal();                  /* the STOP ALARM button */

const AlarmRuntime &runtime(uint8_t id);

/* Pure evaluation of one alarm — used by update() and by the tests. */
void evaluate(const AlarmSettings &s, float value, bool valid, uint32_t now,
              AlarmRuntime &rt, bool &fireLocal, bool &fireRemote);

/* "Temperature exceeded threshold 35.0 C (measured 36.2 C)" */
String payloadText(uint8_t id, float threshold, float value);

}  // namespace AlarmManager

#endif /* ALARM_MANAGER_H */
