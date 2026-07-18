/*
 * App.cpp — application orchestrator of the Alarm Device.
 *
 * Crash recovery strategy:
 *   - all settings, alarm arming and unsent alarms live in NVM and are
 *     restored on every boot (Storage/AlarmNet::begin)
 *   - the reset reason is logged at startup
 *   - an optional task watchdog (APP_WATCHDOG_S) reboots the device if
 *     the main loop ever hangs; blocking UI code keeps it fed through
 *     backgroundTick()
 *   - no dynamic allocation in steady state: modules use static storage,
 *     network clients are stack-scoped per request; the free heap is
 *     logged periodically so a leak would be visible in the debug log
 */
#include "App.h"
#include "AppConfig.h"
#include "DebugLog.h"
#include "Storage.h"
#include "Sensors.h"
#include "AlarmSiren.h"
#include "AlarmManager.h"
#include "AlarmNet.h"
#include "Ui.h"
#include <SensairBoard.h>
#include <esp_task_wdt.h>

SensairDisplay gDisplay;
SensairTouch gTouch;

namespace App {

static SensairBoard board;

static const char *resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "power-on";
    case ESP_RST_SW:        return "software reset";
    case ESP_RST_PANIC:     return "panic/crash";
    case ESP_RST_INT_WDT:   return "interrupt watchdog";
    case ESP_RST_TASK_WDT:  return "task watchdog";
    case ESP_RST_WDT:       return "other watchdog";
    case ESP_RST_BROWNOUT:  return "brownout";
    case ESP_RST_DEEPSLEEP: return "deep sleep wake";
    default:                return "unknown";
  }
}

static void wdtStart() {
#if APP_WATCHDOG_S > 0
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms = (uint32_t)APP_WATCHDOG_S * 1000;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  esp_err_t err = esp_task_wdt_init(&cfg);
  if (err == ESP_ERR_INVALID_STATE) {
    err = esp_task_wdt_reconfigure(&cfg);
  }
  if (err == ESP_OK && esp_task_wdt_add(NULL) == ESP_OK) {
    LOGI("APP", "task watchdog armed: %d s", APP_WATCHDOG_S);
  } else {
    LOGW("APP", "task watchdog setup failed (%d)", (int)err);
  }
#endif
}

static void wdtFeed() {
#if APP_WATCHDOG_S > 0
  esp_task_wdt_reset();
#endif
}

void begin() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  LOGI("APP", "==== Alarm Device starting ====");
  LOGI("APP", "reset reason: %s", resetReasonName(esp_reset_reason()));
  LOGI("APP", "free heap: %lu bytes", (unsigned long)ESP.getFreeHeap());

  board.begin();

  gDisplay.begin();
  gDisplay.setRotation(1);
  if (!gTouch.begin()) {
    LOGE("APP", "touch controller not found — UI will not react!");
  }
  gTouch.setRotation(gDisplay.rotation());

  Storage::begin();
  Sensors::begin();
  AlarmSiren::begin();
  AlarmManager::begin();
  AlarmNet::begin();
  Ui::begin();

  wdtStart();
  LOGI("APP", "startup complete, free heap: %lu",
       (unsigned long)ESP.getFreeHeap());
}

void loop() {
  uint32_t now = millis();

  Sensors::tick(now);
  AlarmManager::update(now);
  AlarmSiren::tick(now);
  AlarmNet::tick(now);
  Ui::tick(now);

  wdtFeed();

  /* heap watermark log — a steady decline here would reveal a leak */
  static uint32_t nextHeapLog = 0;
  if ((int32_t)(now - nextHeapLog) >= 0) {
    nextHeapLog = now + 60000;
    LOGI("APP", "free heap: %lu (min %lu)", (unsigned long)ESP.getFreeHeap(),
         (unsigned long)ESP.getMinFreeHeap());
  }

  delay(5);
}

void backgroundTick() {
  AlarmSiren::tick(millis());
  wdtFeed();
}

}  // namespace App
