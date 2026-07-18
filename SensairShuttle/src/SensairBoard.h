/*
 * SensairBoard.h — board-level helpers for the ESP-SensairShuttle
 *
 * Handles the shared plumbing that the other modules rely on:
 *   - peripheral power rail (LCD + audio, GPIO5)
 *   - the shared I2C bus (GPIO2 = SDA, GPIO3 = SCL)
 *   - BOOT button
 *   - audio power amplifier enable
 *   - an I2C scanner that labels the devices found on this board
 *
 * Every sensor/display module can also be used standalone, but calling
 * SensairBoard::begin() once at the top of setup() is the recommended way
 * to bring the board up.
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_BOARD_H
#define SENSAIR_BOARD_H

#include <Arduino.h>
#include <Wire.h>
#include "SensairPins.h"

class SensairBoard {
public:
    /*
     * Power the display/audio rail, start the shared I2C bus and
     * configure the BOOT button. Safe to call more than once.
     */
    bool begin(TwoWire &wire = Wire, uint32_t i2cFreq = SENSAIR_I2C_FREQ_DEFAULT);

    /* LCD_3V3 + AUDIO_3V3 rail (true = powered). On by default at boot. */
    void setPeripheralPower(bool on);

    /* NS4150B speaker amplifier enable (off after begin()). */
    void setAudioAmp(bool on);

    /* BOOT button, true while pressed. Note: shared with shuttle pin G1. */
    bool buttonPressed() const;

    /*
     * Scan the I2C bus and print one line per device found, with the
     * likely device name for the known addresses on this board.
     * Returns the number of devices found.
     */
    uint8_t i2cScan(Stream &out = Serial);

    /* The bus passed to begin() (Wire by default). */
    TwoWire &wire() { return *_wire; }

private:
    TwoWire *_wire = &Wire;
};

#endif /* SENSAIR_BOARD_H */
