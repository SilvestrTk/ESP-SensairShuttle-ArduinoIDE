/*
 * SensairMic.cpp — analog microphone level meter for the ESP-SensairShuttle
 * Part of the SensairShuttle Arduino library.
 */
#include "SensairMic.h"
#include <math.h>

bool SensairMic::begin() {
    /* Mic power comes from the gated AUDIO_3V3 rail. */
    pinMode(SENSAIR_PIN_PWR_CTRL, OUTPUT);
    digitalWrite(SENSAIR_PIN_PWR_CTRL, LOW);

    /* Full 0..3.3 V input range for the op-amp output. */
    analogSetPinAttenuation(SENSAIR_PIN_MIC_ADC, ADC_11db);
    analogReadMilliVolts(SENSAIR_PIN_MIC_ADC);  /* prime ADC + calibration */

    delay(50);  /* let the bias network start charging */
    return true;
}

SensairSoundLevel SensairMic::readLevel(uint16_t windowMs) {
    SensairSoundLevel out;

    uint32_t tEnd = millis() + windowMs;
    uint32_t n = 0;
    uint64_t sum = 0;
    uint64_t sumSq = 0;
    uint16_t vMin = 0xFFFF, vMax = 0;

    while (millis() < tEnd) {
        uint16_t mv = (uint16_t)analogReadMilliVolts(SENSAIR_PIN_MIC_ADC);
        sum += mv;
        sumSq += (uint32_t)mv * mv;
        if (mv < vMin) vMin = mv;
        if (mv > vMax) vMax = mv;
        n++;
    }
    if (n == 0) return out;

    float mean = (float)sum / n;
    /* RMS of the AC component: sqrt(E[x^2] - mean^2) */
    float meanSq = (float)sumSq / n;
    float var = meanSq - mean * mean;
    out.dcMv = mean;
    out.rmsMv = var > 0 ? sqrtf(var) : 0;
    out.peakMv = max((float)vMax - mean, mean - (float)vMin);
    out.samples = (uint16_t)min(n, (uint32_t)0xFFFF);
    return out;
}
