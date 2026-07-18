/*
 * SensairTouch.h — CST816S capacitive touch panel on the ESP-SensairShuttle
 *
 * The touch controller sits on the LCD flex cable, shares the main I2C bus
 * (SDA=GPIO2, SCL=GPIO3) at address 0x15, and is powered from the gated
 * LCD rail (PWR_CTRL, GPIO5). Neither its interrupt nor its reset line is
 * wired to the ESP32-C5, so the panel is polled.
 *
 * Raw coordinates are in panel-native portrait orientation, x 0..239,
 * y 0..283. Call setRotation() with the same value you passed to
 * SensairDisplay::setRotation() to get display-space coordinates.
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_TOUCH_H
#define SENSAIR_TOUCH_H

#include <Arduino.h>
#include <Wire.h>
#include "SensairPins.h"

enum SensairGesture : uint8_t {
    SENSAIR_GESTURE_NONE        = 0x00,
    SENSAIR_GESTURE_SWIPE_UP    = 0x01,
    SENSAIR_GESTURE_SWIPE_DOWN  = 0x02,
    SENSAIR_GESTURE_SWIPE_LEFT  = 0x03,
    SENSAIR_GESTURE_SWIPE_RIGHT = 0x04,
    SENSAIR_GESTURE_SINGLE_TAP  = 0x05,
    SENSAIR_GESTURE_DOUBLE_TAP  = 0x0B,
    SENSAIR_GESTURE_LONG_PRESS  = 0x0C,
};

struct SensairTouchPoint {
    bool touched = false;       /* finger currently on the panel */
    int16_t x = 0;              /* display-space (after rotation mapping) */
    int16_t y = 0;
    int16_t rawX = 0;           /* panel-native portrait coordinates */
    int16_t rawY = 0;
    SensairGesture gesture = SENSAIR_GESTURE_NONE;
};

class SensairTouch {
public:
    /*
     * Starts I2C (if not already started), powers the LCD rail and probes
     * the controller. Returns true when the CST816S responds.
     */
    bool begin(TwoWire &wire = Wire, uint32_t i2cFreq = SENSAIR_I2C_FREQ_DEFAULT);

    /* Match the display rotation (0..3) so x/y land in display space. */
    void setRotation(uint8_t r) { _rot = r & 3; }

    /*
     * Poll the controller. Returns true (and fills `point`) when a finger
     * is on the panel or a gesture was reported.
     */
    bool read(SensairTouchPoint &point);

    /* Chip ID reported at begin() (CST816S is typically 0xB4/0xB5/0xB6). */
    uint8_t chipId() const { return _chipId; }

    static const char *gestureName(SensairGesture g);

private:
    bool readRegs(uint8_t reg, uint8_t *buf, size_t len);
    bool writeReg(uint8_t reg, uint8_t value);

    TwoWire *_wire = &Wire;
    uint8_t _rot = 0;
    uint8_t _chipId = 0;
};

#endif /* SENSAIR_TOUCH_H */
