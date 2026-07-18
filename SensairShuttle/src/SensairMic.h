/*
 * SensairMic.h — microphone input on the ESP-SensairShuttle
 *
 * The board's microphone is an analog electret capsule amplified by an
 * LMV321 op-amp; the amplified signal (OPA_OUT) goes to GPIO6 = ADC.
 * The mic is powered from the gated AUDIO_3V3 rail (PWR_CTRL = GPIO5,
 * LOW = on) and sits on a DC bias of roughly half the rail — this module
 * measures the actual bias at runtime and reports the AC component.
 *
 * This is a LEVEL meter, not an audio recorder: readLevel() samples the
 * ADC in a tight loop (several kHz effective) over a short window and
 * returns bias, RMS and peak in millivolts — plenty for sound detection,
 * clap detection or a VU meter. Note: the bias network takes a moment to
 * charge after power-up, so the first ~300 ms of readings drift.
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_MIC_H
#define SENSAIR_MIC_H

#include <Arduino.h>
#include "SensairPins.h"

struct SensairSoundLevel {
    float dcMv = 0;    /* DC bias the signal is centered on */
    float rmsMv = 0;   /* RMS of the AC component — the "loudness" */
    float peakMv = 0;  /* largest deviation from the bias in the window */
    uint16_t samples = 0;
};

class SensairMic {
public:
    /* Powers the audio rail and configures the ADC pin. */
    bool begin();

    /*
     * Sample the microphone for windowMs milliseconds (default 30 ms,
     * blocking) and return the measured levels.
     */
    SensairSoundLevel readLevel(uint16_t windowMs = 30);

    /* True when the RMS level in the window exceeds thresholdMv. */
    bool soundDetected(float thresholdMv = 40.0f, uint16_t windowMs = 30) {
        return readLevel(windowMs).rmsMv >= thresholdMv;
    }
};

#endif /* SENSAIR_MIC_H */
