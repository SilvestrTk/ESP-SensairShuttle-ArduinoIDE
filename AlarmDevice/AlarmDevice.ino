/*
 * AlarmDevice.ino — multi-sensor alarm device for the ESP-SensairShuttle.
 *
 * All configuration lives in AppConfig.h; the application logic lives in
 * the modules (see README.md). Set APP_RUN_TESTS to 1 in AppConfig.h to
 * build the self-test suite instead of the application.
 *
 * Board setup: "ESP32C5 Dev Module", USB CDC On Boot: Enabled.
 * Requires the SensairShuttle library.
 */
#include "AppConfig.h"

#if APP_RUN_TESTS

#include "Tests.h"
void setup() { Tests::run(); }
void loop() { delay(1000); }

#else

#include "App.h"
void setup() { App::begin(); }
void loop() { App::loop(); }

#endif
