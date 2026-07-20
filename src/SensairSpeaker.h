/*
 * SensairSpeaker.h — speaker output on the ESP-SensairShuttle
 *
 * The board drives the speaker with a differential PDM signal
 * (PDM_P = GPIO7, PDM_N = GPIO8) through an NS4150B class-D amplifier
 * (enable = GPIO1, gain ~7.2). This module reproduces the factory
 * firmware's setup: I2S peripheral in PDM TX mode on the P pin, with the
 * hardware-inverted copy of the same signal routed to the N pin through
 * the GPIO matrix. The amplifier is powered from the gated AUDIO_3V3
 * rail (PWR_CTRL = GPIO5, LOW = on).
 *
 * Audio format: mono, 16-bit signed PCM at the sample rate passed to
 * begin() (default 16 kHz).
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_SPEAKER_H
#define SENSAIR_SPEAKER_H

#include <Arduino.h>
#include "SensairPins.h"
#include "driver/i2s_pdm.h"

class SensairSpeaker {
public:
    /* Set up the PDM output. Returns true on success. */
    bool begin(uint32_t sampleRate = 16000);

    /* Release the I2S peripheral and switch the amplifier off. */
    void end();

    /*
     * Play a sine tone (blocking). volume 0..100. A short fade-in/out is
     * applied to avoid clicks. The amplifier is enabled automatically on
     * first playback.
     */
    bool playTone(uint16_t freqHz, uint16_t durationMs, uint8_t volume = 60);

    /* Convenience: short default beep. */
    bool beep(uint16_t freqHz = 1500, uint16_t durationMs = 80) {
        return playTone(freqHz, durationMs);
    }

    /* Play raw mono 16-bit PCM samples at the begin() sample rate (blocking). */
    bool playSamples(const int16_t *samples, size_t count);

    /* NS4150B amplifier enable. Managed automatically during playback,
     * but can be forced off to eliminate any idle hiss. */
    void setAmp(bool on);

    uint32_t sampleRate() const { return _rate; }
    bool isReady() const { return _ready; }

private:
    i2s_chan_handle_t _tx = nullptr;
    uint32_t _rate = 16000;
    bool _ready = false;
    bool _ampOn = false;
};

#endif /* SENSAIR_SPEAKER_H */
