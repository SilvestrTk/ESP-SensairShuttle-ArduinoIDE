/*
 * SensairBMM350.cpp — BMM350 direct (main bus) or via BMI270 aux interface
 * Part of the SensairShuttle Arduino library.
 */
#include "SensairBMM350.h"

/* ------------------------------------------------ direct main-bus mode --- */

bool SensairBMM350::begin(TwoWire &wire, uint32_t i2cFreq, uint8_t addr) {
    _bmi = nullptr;
    _ready = false;
    _link.wire = &wire;
    _link.addr = addr;

    wire.begin(SENSAIR_PIN_I2C_SDA, SENSAIR_PIN_I2C_SCL, i2cFreq);

    /* Fast presence check: on older shuttle revisions the magnetometer is
     * only reachable through the BMI270 aux interface, not here. */
    wire.beginTransmission(addr);
    if (wire.endTransmission() != 0) {
        _rslt = BMM350_E_DEV_NOT_FOUND;
        return false;
    }

    _dev = bmm350_dev();
    _dev.intf_ptr = &_link;
    _dev.read = i2cRead;
    _dev.write = i2cWrite;
    _dev.delay_us = delayUs;
    return finishInit();
}

/* ------------------------------------------------ aux tunnel mode -------- */

bool SensairBMM350::begin(SensairBMI270 &imu) {
    _ready = false;
    if (!imu.isReady()) return false;
    _bmi = imu.dev();

    /* Configure the BMI270 auxiliary I2C master in manual mode, pointed at
     * the BMM350 (address 0x14). Mirrors the Espressif factory firmware. */
    struct bmi2_sens_config cfg;
    cfg.type = BMI2_AUX;
    int8_t rslt = bmi2_get_sensor_config(&cfg, 1, _bmi);
    if (rslt != BMI2_OK) { _rslt = rslt; return false; }

    cfg.cfg.aux.aux_en = BMI2_ENABLE;
    cfg.cfg.aux.manual_en = BMI2_ENABLE;
    cfg.cfg.aux.fcu_write_en = BMI2_ENABLE;
    cfg.cfg.aux.man_rd_burst = BMI2_AUX_READ_LEN_3;  /* 8-byte bursts */
    cfg.cfg.aux.odr = BMI2_AUX_ODR_25HZ;
    cfg.cfg.aux.read_addr = BMM350_REG_MAG_X_XLSB;
    cfg.cfg.aux.i2c_device_addr = SENSAIR_ADDR_BMM350_AUX;
    rslt = bmi2_set_sensor_config(&cfg, 1, _bmi);
    if (rslt != BMI2_OK) { _rslt = rslt; return false; }

    uint8_t sensors[1] = { BMI2_AUX };
    rslt = bmi2_sensor_enable(sensors, 1, _bmi);
    if (rslt != BMI2_OK) { _rslt = rslt; return false; }
    delay(5);

    /* The BMM350 driver talks through the aux tunnel from here on. */
    _dev = bmm350_dev();
    _dev.intf_ptr = _bmi;
    _dev.read = auxRead;
    _dev.write = auxWrite;
    _dev.delay_us = delayUs;
    return finishInit();
}

/* ------------------------------------- shared init tail (both modes) ----- */

bool SensairBMM350::finishInit() {
    _rslt = bmm350_init(&_dev);
    if (_rslt != BMM350_OK) return false;

    _rslt = bmm350_magnetic_reset_and_wait(&_dev);
    if (_rslt != BMM350_OK) return false;

    _rslt = bmm350_set_odr_performance(BMM350_DATA_RATE_25HZ, BMM350_REGULARPOWER, &_dev);
    if (_rslt != BMM350_OK) return false;

    _rslt = bmm350_enable_axes(BMM350_X_EN, BMM350_Y_EN, BMM350_Z_EN, &_dev);
    if (_rslt != BMM350_OK) return false;

    _rslt = bmm350_set_powermode(BMM350_NORMAL_MODE, &_dev);
    if (_rslt != BMM350_OK) return false;

    /* First conversion needs one ODR period. */
    delay(50);
    _ready = true;
    return true;
}

bool SensairBMM350::read(SensairMagData &data) {
    if (!_ready) return false;
    struct bmm350_mag_temp_data raw;
    _rslt = bmm350_get_compensated_mag_xyz_temp_data(&raw, &_dev);
    if (_rslt != BMM350_OK) return false;
    data.x = raw.x;
    data.y = raw.y;
    data.z = raw.z;
    data.temperature = raw.temperature;
    return true;
}

float SensairBMM350::heading(const SensairMagData &d) {
    float h = atan2f(d.y, d.x) * 180.0f / (float)M_PI;
    if (h < 0) h += 360.0f;
    return h;
}

float SensairBMM350::fieldStrength(const SensairMagData &d) {
    return sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
}

bool SensairBMM350::magneticReset() {
    if (!_ready) return false;
    _rslt = bmm350_magnetic_reset_and_wait(&_dev);
    return _rslt == BMM350_OK;
}

/* --------------------------------------------- direct main-bus glue ------ */

int8_t SensairBMM350::i2cRead(uint8_t reg, uint8_t *data, uint32_t len, void *intf) {
    I2cLink *l = (I2cLink *)intf;
    l->wire->beginTransmission(l->addr);
    l->wire->write(reg);
    if (l->wire->endTransmission(false) != 0) return BMM350_E_COM_FAIL;
    uint32_t got = 0;
    while (got < len) {
        uint8_t chunk = (uint8_t)min(len - got, (uint32_t)32);
        bool last = (got + chunk) == len;
        uint8_t n = l->wire->requestFrom(l->addr, chunk, (uint8_t)last);
        if (n == 0) return BMM350_E_COM_FAIL;
        while (n-- && got < len) data[got++] = l->wire->read();
    }
    return BMM350_OK;
}

int8_t SensairBMM350::i2cWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *intf) {
    I2cLink *l = (I2cLink *)intf;
    l->wire->beginTransmission(l->addr);
    l->wire->write(reg);
    l->wire->write(data, len);
    return l->wire->endTransmission() == 0 ? BMM350_OK : BMM350_E_COM_FAIL;
}

/* --------------------------------------------- aux tunnel through BMI270 - */

int8_t SensairBMM350::auxRead(uint8_t reg, uint8_t *data, uint32_t len, void *intf) {
    return bmi2_read_aux_man_mode(reg, data, (uint16_t)len, (struct bmi2_dev *)intf);
}

int8_t SensairBMM350::auxWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *intf) {
    return bmi2_write_aux_man_mode(reg, data, (uint16_t)len, (struct bmi2_dev *)intf);
}

void SensairBMM350::delayUs(uint32_t us, void *) {
    if (us >= 1000) {
        delay(us / 1000);
        us %= 1000;
    }
    if (us) delayMicroseconds(us);
}
