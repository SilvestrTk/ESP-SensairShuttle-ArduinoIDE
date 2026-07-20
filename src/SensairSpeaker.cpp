/*
 * SensairSpeaker.cpp — differential PDM speaker output for the ESP-SensairShuttle
 * Setup mirrors the Espressif factory firmware (esp-dev-kits, setup_device.c).
 * Part of the SensairShuttle Arduino library.
 */
#include "SensairSpeaker.h"
#include "soc/gpio_sig_map.h"
#include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include <math.h>

bool SensairSpeaker::begin(uint32_t sampleRate) {
    if (_ready) return true;
    _rate = sampleRate;

    /* Audio rail on (shared with LCD/touch), amplifier off for now. */
    pinMode(SENSAIR_PIN_PWR_CTRL, OUTPUT);
    digitalWrite(SENSAIR_PIN_PWR_CTRL, LOW);
    pinMode(SENSAIR_PIN_PA_ENABLE, OUTPUT);
    digitalWrite(SENSAIR_PIN_PA_ENABLE, LOW);
    _ampOn = false;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    if (i2s_new_channel(&chan_cfg, &_tx, nullptr) != ESP_OK) return false;

    i2s_pdm_tx_config_t cfg = {};
    cfg.clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(_rate);
    cfg.clk_cfg.up_sample_fs = 480;          /* factory firmware value */
    cfg.slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    cfg.gpio_cfg.clk = GPIO_NUM_NC;          /* PDM needs no clock pin here */
    cfg.gpio_cfg.dout = (gpio_num_t)SENSAIR_PIN_PDM_P;
#if SOC_I2S_PDM_MAX_TX_LINES > 1
    cfg.gpio_cfg.dout2 = GPIO_NUM_NC;
#endif
    cfg.gpio_cfg.invert_flags.clk_inv = false;

    if (i2s_channel_init_pdm_tx_mode(_tx, &cfg) != ESP_OK) {
        i2s_del_channel(_tx);
        _tx = nullptr;
        return false;
    }
    if (i2s_channel_enable(_tx) != ESP_OK) {
        i2s_del_channel(_tx);
        _tx = nullptr;
        return false;
    }

    /* Differential drive: route the INVERTED I2S data signal to the N pin
     * through the GPIO matrix — same trick as the factory firmware. */
    gpio_set_direction((gpio_num_t)SENSAIR_PIN_PDM_N, GPIO_MODE_OUTPUT);
    esp_rom_gpio_connect_out_signal((gpio_num_t)SENSAIR_PIN_PDM_N,
                                    I2SO_SD_OUT_IDX, true /* invert */, false);

    /* Low drive strength into the amplifier's RC input filter. */
    gpio_set_drive_capability((gpio_num_t)SENSAIR_PIN_PDM_P, GPIO_DRIVE_CAP_0);
    gpio_set_drive_capability((gpio_num_t)SENSAIR_PIN_PDM_N, GPIO_DRIVE_CAP_0);

    _ready = true;
    return true;
}

void SensairSpeaker::end() {
    setAmp(false);
    if (_tx) {
        i2s_channel_disable(_tx);
        i2s_del_channel(_tx);
        _tx = nullptr;
    }
    _ready = false;
}

void SensairSpeaker::setAmp(bool on) {
    pinMode(SENSAIR_PIN_PA_ENABLE, OUTPUT);
    digitalWrite(SENSAIR_PIN_PA_ENABLE, on ? HIGH : LOW);
    if (on && !_ampOn) {
        delay(15);  /* NS4150B startup before first samples, avoids a pop */
    }
    _ampOn = on;
}

bool SensairSpeaker::playTone(uint16_t freqHz, uint16_t durationMs, uint8_t volume) {
    if (!_ready || freqHz == 0) return false;
    setAmp(true);

    if (volume > 100) volume = 100;
    const float amp = 32767.0f * volume / 100.0f;
    const float dp = 2.0f * (float)M_PI * freqHz / _rate;
    const uint32_t total = (uint32_t)_rate * durationMs / 1000;
    uint32_t fade = _rate * 5 / 1000;   /* 5 ms fade in/out against clicks */
    if (fade * 2 > total) fade = total / 2;

    int16_t buf[256];
    float phase = 0;
    uint32_t n = 0;
    while (n < total) {
        size_t chunk = min((uint32_t)256, total - n);
        for (size_t i = 0; i < chunk; i++) {
            float a = amp;
            uint32_t idx = n + i;
            if (idx < fade) a *= (float)idx / fade;
            else if (total - idx <= fade) a *= (float)(total - idx) / fade;
            buf[i] = (int16_t)(a * sinf(phase));
            phase += dp;
            if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
        }
        size_t written = 0;
        if (i2s_channel_write(_tx, buf, chunk * sizeof(int16_t), &written, 1000) != ESP_OK) {
            return false;
        }
        n += chunk;
    }
    return true;
}

bool SensairSpeaker::playSamples(const int16_t *samples, size_t count) {
    if (!_ready || !samples || !count) return false;
    setAmp(true);
    size_t written = 0;
    return i2s_channel_write(_tx, samples, count * sizeof(int16_t), &written, 5000) == ESP_OK;
}
