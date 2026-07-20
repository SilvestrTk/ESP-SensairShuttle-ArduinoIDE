/*
 * SensairBoard.cpp — board-level helpers for the ESP-SensairShuttle
 * Part of the SensairShuttle Arduino library.
 */
#include "SensairBoard.h"

bool SensairBoard::begin(TwoWire &wire, uint32_t i2cFreq) {
    _wire = &wire;

    /* Peripheral power on (display, touch, audio). Default-on in hardware,
     * but drive it explicitly so a previous sketch can't leave it off. */
    setPeripheralPower(true);

    /* Speaker amplifier off until someone wants it. */
    pinMode(SENSAIR_PIN_PA_ENABLE, OUTPUT);
    digitalWrite(SENSAIR_PIN_PA_ENABLE, LOW);

    /* BOOT button (has external circuitry, input only). */
    pinMode(SENSAIR_PIN_BOOT_BUTTON, INPUT_PULLUP);

    /* Shared I2C bus. begin() is safe to call repeatedly on the ESP32 core. */
    bool ok = _wire->begin(SENSAIR_PIN_I2C_SDA, SENSAIR_PIN_I2C_SCL, i2cFreq);

    /* Give the CST816S / sensors time to come out of power-on reset. */
    delay(50);
    return ok;
}

void SensairBoard::setPeripheralPower(bool on) {
    pinMode(SENSAIR_PIN_PWR_CTRL, OUTPUT);
    /* P-MOSFET high-side switch: LOW = rail on. */
    digitalWrite(SENSAIR_PIN_PWR_CTRL, on ? LOW : HIGH);
    if (on) {
        delay(20); /* rail settle time before talking to the LCD/touch */
    }
}

void SensairBoard::setAudioAmp(bool on) {
    pinMode(SENSAIR_PIN_PA_ENABLE, OUTPUT);
    digitalWrite(SENSAIR_PIN_PA_ENABLE, on ? HIGH : LOW);
}

bool SensairBoard::buttonPressed() const {
    return digitalRead(SENSAIR_PIN_BOOT_BUTTON) == LOW;
}

uint8_t SensairBoard::i2cScan(Stream &out) {
    uint8_t found = 0;
    out.println(F("I2C scan (SDA=GPIO2, SCL=GPIO3):"));
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        _wire->beginTransmission(addr);
        if (_wire->endTransmission() == 0) {
            found++;
            const char *name = "unknown device";
            switch (addr) {
                case SENSAIR_ADDR_TOUCH:      name = "CST816S touch panel"; break;
                case SENSAIR_ADDR_TOUCH_BTN:  name = "BS8112 touch buttons"; break;
                case SENSAIR_ADDR_BME690:
                case SENSAIR_ADDR_BME690_ALT: name = "BME690 environmental sensor"; break;
                case SENSAIR_ADDR_BMI270:
                case SENSAIR_ADDR_BMI270_ALT: name = "BMI270 IMU"; break;
                default: break;
            }
            out.printf("  0x%02X  %s\r\n", addr, name);
        }
    }
    if (found == 0) {
        out.println(F("  no devices found"));
    }
    out.println(F("Note: the BMM350 magnetometer does not appear here — it sits"));
    out.println(F("behind the BMI270 auxiliary interface (see SensairBMM350)."));
    return found;
}
