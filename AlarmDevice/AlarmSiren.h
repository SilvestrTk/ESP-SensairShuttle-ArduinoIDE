/*
 * AlarmSiren.h — the ONE entity controlling the speaker.
 *
 * Any number of local alarms may be active in parallel; they all just
 * request the siren via setActive(). The siren plays a two-tone pattern
 * (non-blocking between beeps) until every local alarm is stopped.
 * No other module may touch the speaker.
 */
#ifndef ALARM_SIREN_H
#define ALARM_SIREN_H

#include <Arduino.h>

namespace AlarmSiren {

void begin();
void setActive(bool on);
bool isActive();
void tick(uint32_t now);   /* call often — also from blocking UI loops */

/* short UI feedback beep (only when the siren is not sounding) */
void chirp();

}  // namespace AlarmSiren

#endif /* ALARM_SIREN_H */
