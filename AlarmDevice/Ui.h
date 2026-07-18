/*
 * Ui.h — touchscreen user interface of the Alarm Device.
 *
 * Screens:
 *   MAIN        live values of all alarm sources, ALARMS / CONNECT
 *               buttons, and the STOP ALARM overlay while a local alarm
 *               is sounding (the UI jumps here automatically)
 *   ALARM LIST  one row per alarm with its configuration summary
 *   ALARM EDIT  threshold +/- , local & remote checkboxes, reset time
 *   CONNECTION  WiFi setup, server URL, device ID (rotary keyboard)
 *   WIFI SCAN   network list -> password entry -> connect & save
 */
#ifndef UI_H
#define UI_H

#include <Arduino.h>

namespace Ui {

void begin();
void tick(uint32_t now);

}  // namespace Ui

#endif /* UI_H */
