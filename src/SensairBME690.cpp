/*
 * SensairBME690.cpp — BME690 wrapper for the ESP-SensairShuttle
 * Part of the SensairShuttle Arduino library.
 */
#include "SensairBME690.h"

bool SensairBME690::begin(TwoWire &wire, uint32_t i2cFreq, uint8_t addr) {
    _link.wire = &wire;
    _link.addr = addr;

    /* Shuttle select pins. IMPORTANT: these lines pass through a PCA9306
     * level shifter to the sensor's 1.8 V rail. NEVER drive them HIGH at
     * 3.3 V — that overdrives the sensor pin through the shifter and knocks
     * the sensor off the I2C bus. Drive LOW, or float and let the on-board
     * pull-ups provide the high level. */
    pinMode(SENSAIR_PIN_SHUTTLE_CS, INPUT);   /* floats high -> I2C mode */
    if (addr == SENSAIR_ADDR_BME690) {
        pinMode(SENSAIR_PIN_SHUTTLE_SDO, OUTPUT);   /* SDO low -> 0x76 */
        digitalWrite(SENSAIR_PIN_SHUTTLE_SDO, LOW);
    } else {
        pinMode(SENSAIR_PIN_SHUTTLE_SDO, INPUT);    /* floats high -> 0x77 */
    }
    delay(5);

    wire.begin(SENSAIR_PIN_I2C_SDA, SENSAIR_PIN_I2C_SCL, i2cFreq);

    /* Zero-length probe: fail fast when nothing ACKs at this address.
     * Also required for bus health — a NACKed *data* write to an absent
     * device can leave the ESP32 I2C driver in a bad state for the next
     * transaction, whereas this probe maps to a clean i2c_master_probe. */
    wire.beginTransmission(addr);
    if (wire.endTransmission() != 0) {
        _rslt = BME69X_E_DEV_NOT_FOUND;
        return false;
    }

    _dev.intf = BME69X_I2C_INTF;
    _dev.intf_ptr = &_link;
    _dev.read = i2cRead;
    _dev.write = i2cWrite;
    _dev.delay_us = delayUs;
    _dev.amb_temp = 25;

    _rslt = bme69x_init(&_dev);
    if (_rslt != BME69X_OK) return false;

    /* Default measurement configuration. */
    _conf.os_hum = BME69X_OS_1X;
    _conf.os_temp = BME69X_OS_2X;
    _conf.os_pres = BME69X_OS_16X;
    _conf.filter = BME69X_FILTER_OFF;
    _conf.odr = BME69X_ODR_NONE;
    _rslt = bme69x_set_conf(&_conf, &_dev);
    if (_rslt != BME69X_OK) return false;

    _ready = true;
    return setHeater(300, 100);
}

bool SensairBME690::setHeater(uint16_t tempC, uint16_t durationMs) {
    if (!_ready) return false;
    _heatr.enable = durationMs ? BME69X_ENABLE : BME69X_DISABLE;
    _heatr.heatr_temp = tempC;
    _heatr.heatr_dur = durationMs;
    _rslt = bme69x_set_heatr_conf(BME69X_FORCED_MODE, &_heatr, &_dev);
    return _rslt == BME69X_OK;
}

bool SensairBME690::read(SensairEnvData &data) {
    if (!_ready) return false;

    _rslt = bme69x_set_op_mode(BME69X_FORCED_MODE, &_dev);
    if (_rslt != BME69X_OK) return false;

    /* Wait for the measurement (TPH conversion + heater time). */
    uint32_t us = bme69x_get_meas_dur(BME69X_FORCED_MODE, &_conf, &_dev)
                  + (uint32_t)_heatr.heatr_dur * 1000UL;
    delayUs(us, nullptr);

    struct bme69x_data raw;
    uint8_t nFields = 0;
    _rslt = bme69x_get_data(BME69X_FORCED_MODE, &raw, &nFields, &_dev);
    if (_rslt != BME69X_OK || nFields == 0) return false;

    data.temperature = raw.temperature;
    data.humidity = raw.humidity;
    data.pressure = raw.pressure / 100.0f;  /* Pa -> hPa */
    data.gasResistance = raw.gas_resistance;
    data.gasValid = raw.status & BME69X_GASM_VALID_MSK;
    data.heaterStable = raw.status & BME69X_HEAT_STAB_MSK;
    return true;
}

/* ------------------------------------------------------- Bosch API glue --- */

BME69X_INTF_RET_TYPE SensairBME690::i2cRead(uint8_t reg, uint8_t *data, uint32_t len, void *intf) {
    I2cLink *l = (I2cLink *)intf;
    l->wire->beginTransmission(l->addr);
    l->wire->write(reg);
    if (l->wire->endTransmission(false) != 0) return -1;
    uint32_t got = 0;
    while (got < len) {
        uint8_t chunk = (uint8_t)min(len - got, (uint32_t)32);
        bool last = (got + chunk) == len;
        uint8_t n = l->wire->requestFrom(l->addr, chunk, (uint8_t)last);
        if (n == 0) return -1;
        while (n-- && got < len) data[got++] = l->wire->read();
    }
    return 0;
}

BME69X_INTF_RET_TYPE SensairBME690::i2cWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *intf) {
    I2cLink *l = (I2cLink *)intf;
    l->wire->beginTransmission(l->addr);
    l->wire->write(reg);
    l->wire->write(data, len);
    return l->wire->endTransmission() == 0 ? 0 : -1;
}

void SensairBME690::delayUs(uint32_t us, void *) {
    if (us >= 1000) {
        delay(us / 1000);
        us %= 1000;
    }
    if (us) delayMicroseconds(us);
}
