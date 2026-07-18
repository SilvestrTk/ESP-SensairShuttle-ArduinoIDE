/*
 * App.h — application orchestrator + shared hardware objects.
 *
 * The display and touch objects are shared by Ui and RotaryInput.
 * The speaker is deliberately NOT here — it is owned exclusively by
 * AlarmSiren (single point of control).
 */
#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include <SensairDisplay.h>
#include <SensairTouch.h>

extern SensairDisplay gDisplay;
extern SensairTouch gTouch;

namespace App {

void begin();
void loop();

/*
 * Keep-alive for blocking UI code (rotary input, connect dialogs):
 * feeds the watchdog and keeps the siren pattern running.
 */
void backgroundTick();

}  // namespace App

#endif /* APP_H */
