/*
 * RotaryInput.h — rotating-character text entry (from library example
 * 14_Rotary_Keyboard), packaged as a reusable blocking component.
 *
 * Used for every text value in the app: WiFi password, server URL,
 * device ID. While it runs it repeatedly calls the provided background
 * callback so alarms, the siren and the watchdog stay alive.
 *
 *   String v = Storage::deviceId();
 *   if (RotaryInput::edit("Device ID", v, 32, App::backgroundTick)) {
 *     Storage::setDeviceId(v);
 *   }
 */
#ifndef ROTARY_INPUT_H
#define ROTARY_INPUT_H

#include <Arduino.h>

namespace RotaryInput {

/*
 * Edit `value` in place. Returns true on OK, false on cancel (value is
 * left unchanged on cancel). bgTick may be nullptr.
 */
bool edit(const char *title, String &value, size_t maxLen, void (*bgTick)());

}  // namespace RotaryInput

#endif /* ROTARY_INPUT_H */
