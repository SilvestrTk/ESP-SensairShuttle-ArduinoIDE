/*
 * SensairTouch.cpp — CST816S touch driver for the ESP-SensairShuttle
 * Part of the SensairShuttle Arduino library.
 */
#include "SensairTouch.h"

/* CST816S registers */
#define REG_GESTURE      0x01
#define REG_FINGER_NUM   0x02
#define REG_X_H          0x03  /* bits 3:0 = X[11:8] */
#define REG_CHIP_ID      0xA7
#define REG_MOTION_MASK  0xEC  /* bit0 = enable double-click detection */
#define REG_DIS_AUTOSLEEP 0xFE /* write non-zero to keep the chip awake */

bool SensairTouch::begin(TwoWire &wire, uint32_t i2cFreq) {
    _wire = &wire;

    /* Touch power comes from the gated LCD rail. */
    pinMode(SENSAIR_PIN_PWR_CTRL, OUTPUT);
    digitalWrite(SENSAIR_PIN_PWR_CTRL, LOW);
    delay(60); /* CST816S boot time after power-up */

    _wire->begin(SENSAIR_PIN_I2C_SDA, SENSAIR_PIN_I2C_SCL, i2cFreq);

    /* Probe: the CST816S naps between touches, so retry a few times. */
    bool found = false;
    for (uint8_t attempt = 0; attempt < 5 && !found; attempt++) {
        _wire->beginTransmission(SENSAIR_ADDR_TOUCH);
        found = (_wire->endTransmission() == 0);
        if (!found) delay(20);
    }
    if (!found) return false;

    readRegs(REG_CHIP_ID, &_chipId, 1);

    /* Keep the controller awake — with no interrupt line wired we must be
     * able to poll it at any time. Best effort; ignore failure. */
    writeReg(REG_DIS_AUTOSLEEP, 0x01);

    /* Enable double-click detection in the gesture engine (off by default). */
    writeReg(REG_MOTION_MASK, 0x01);
    return true;
}

bool SensairTouch::read(SensairTouchPoint &point) {
    uint8_t d[6];
    point = SensairTouchPoint();
    if (!readRegs(REG_GESTURE, d, sizeof(d))) return false;

    point.gesture = (SensairGesture)d[0];
    uint8_t fingers = d[1];
    point.touched = fingers > 0;
    if (!point.touched && point.gesture == SENSAIR_GESTURE_NONE) return false;

    int16_t rx = ((d[2] & 0x0F) << 8) | d[3];
    int16_t ry = ((d[4] & 0x0F) << 8) | d[5];

    /* The CST816 on this panel reports touches in a PORTRAIT frame:
     * rawX 0..239 across the short edge, rawY 0..283 along the long edge,
     * with the long axis inverted relative to the display's portrait
     * orientation. Mapping below calibrated on hardware with a four-
     * quadrant button test. */
    if (rx >= SENSAIR_LCD_WIDTH)  rx = SENSAIR_LCD_WIDTH - 1;   /* 240 axis */
    if (ry >= SENSAIR_LCD_HEIGHT) ry = SENSAIR_LCD_HEIGHT - 1;  /* 284 axis */
    point.rawX = rx;
    point.rawY = ry;

    /* Map raw coords into display space (see SensairDisplay::setRotation
     * for the matching MADCTL settings). */
    switch (_rot) {
        case 0: /* portrait */
            point.x = rx;
            point.y = (SENSAIR_LCD_HEIGHT - 1) - ry;
            break;
        case 1: /* landscape, factory orientation — hardware-verified */
            point.x = (SENSAIR_LCD_HEIGHT - 1) - ry;
            point.y = (SENSAIR_LCD_WIDTH - 1) - rx;
            break;
        case 2: /* portrait flipped */
            point.x = (SENSAIR_LCD_WIDTH - 1) - rx;
            point.y = ry;
            break;
        case 3: /* landscape flipped */
            point.x = ry;
            point.y = rx;
            break;
    }
    return true;
}

const char *SensairTouch::gestureName(SensairGesture g) {
    switch (g) {
        case SENSAIR_GESTURE_NONE:        return "none";
        case SENSAIR_GESTURE_SWIPE_UP:    return "swipe up";
        case SENSAIR_GESTURE_SWIPE_DOWN:  return "swipe down";
        case SENSAIR_GESTURE_SWIPE_LEFT:  return "swipe left";
        case SENSAIR_GESTURE_SWIPE_RIGHT: return "swipe right";
        case SENSAIR_GESTURE_SINGLE_TAP:  return "single tap";
        case SENSAIR_GESTURE_DOUBLE_TAP:  return "double tap";
        case SENSAIR_GESTURE_LONG_PRESS:  return "long press";
        default:                          return "unknown";
    }
}

bool SensairTouch::readRegs(uint8_t reg, uint8_t *buf, size_t len) {
    _wire->beginTransmission(SENSAIR_ADDR_TOUCH);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) return false;
    size_t got = _wire->requestFrom((uint8_t)SENSAIR_ADDR_TOUCH, (uint8_t)len);
    if (got != len) return false;
    for (size_t i = 0; i < len; i++) buf[i] = _wire->read();
    return true;
}

bool SensairTouch::writeReg(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(SENSAIR_ADDR_TOUCH);
    _wire->write(reg);
    _wire->write(value);
    return _wire->endTransmission() == 0;
}
