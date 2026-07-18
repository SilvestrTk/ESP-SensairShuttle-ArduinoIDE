/*
 * AlarmSiren.cpp — the single speaker owner of the Alarm Device.
 */
#include "AlarmSiren.h"
#include "AppConfig.h"
#include "DebugLog.h"
#include <SensairSpeaker.h>

namespace AlarmSiren {

static SensairSpeaker speaker;
static bool ready = false;
static bool active = false;
static bool toneA = true;
static uint32_t nextBeep = 0;

void begin() {
  ready = speaker.begin(16000);
  if (ready) {
    speaker.setAmp(false);
    LOGI("SIRN", "speaker ready");
  } else {
    LOGE("SIRN", "speaker init FAILED — local alarms will be silent");
  }
}

void setActive(bool on) {
  if (on == active) return;
  active = on;
  LOGW("SIRN", "siren %s", on ? "ON" : "OFF");
  if (!on && ready) speaker.setAmp(false);
  if (on) nextBeep = 0;  /* beep immediately */
}

bool isActive() { return active; }

void tick(uint32_t now) {
  if (!active || !ready) return;
  if ((int32_t)(now - nextBeep) >= 0) {
    /* the beep itself blocks for APP_SIREN_BEEP_MS — keep it short so
     * the UI stays responsive between beeps */
    speaker.playTone(toneA ? APP_SIREN_FREQ_A : APP_SIREN_FREQ_B,
                     APP_SIREN_BEEP_MS, APP_SIREN_VOLUME);
    toneA = !toneA;
    nextBeep = now + APP_SIREN_BEEP_MS + APP_SIREN_GAP_MS;
  }
}

void chirp() {
  if (ready && !active) {
    speaker.playTone(1800, 30, 40);
    speaker.setAmp(false);
  }
}

}  // namespace AlarmSiren
