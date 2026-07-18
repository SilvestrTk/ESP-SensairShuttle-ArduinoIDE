/*
 * SensairBMI270.cpp — BMI270 wrapper for the ESP-SensairShuttle
 * Part of the SensairShuttle Arduino library.
 */
#include "SensairBMI270.h"

bool SensairBMI270::begin(TwoWire &wire, uint32_t i2cFreq, uint8_t addr) {
    _link.wire = &wire;
    _link.addr = addr;

    /* Shuttle select pins. IMPORTANT: these lines pass through a PCA9306
     * level shifter to the sensor's 1.8 V rail. NEVER drive them HIGH at
     * 3.3 V — that overdrives the sensor pin through the shifter and knocks
     * the sensor off the I2C bus. Drive LOW, or float and let the on-board
     * pull-ups provide the high level. */
    pinMode(SENSAIR_PIN_SHUTTLE_CS, INPUT);   /* floats high -> I2C mode */
    if (addr == SENSAIR_ADDR_BMI270) {
        pinMode(SENSAIR_PIN_SHUTTLE_SDO, OUTPUT);   /* SDO low -> 0x68 */
        digitalWrite(SENSAIR_PIN_SHUTTLE_SDO, LOW);
    } else {
        pinMode(SENSAIR_PIN_SHUTTLE_SDO, INPUT);    /* floats high -> 0x69 */
    }
    delay(5);

    wire.begin(SENSAIR_PIN_I2C_SDA, SENSAIR_PIN_I2C_SCL, i2cFreq);

    /* Zero-length probe: fail fast when nothing ACKs at this address.
     * Also required for bus health — a NACKed *data* write to an absent
     * device can leave the ESP32 I2C driver in a bad state for the next
     * transaction, whereas this probe maps to a clean i2c_master_probe. */
    wire.beginTransmission(addr);
    if (wire.endTransmission() != 0) {
        _rslt = BMI2_E_DEV_NOT_FOUND;
        return false;
    }

    _dev.intf = BMI2_I2C_INTF;
    _dev.intf_ptr = &_link;
    _dev.read = i2cRead;
    _dev.write = i2cWrite;
    _dev.delay_us = delayUs;
    _dev.read_write_len = 32;      /* chunk size fits the Wire buffer */
    _dev.config_file_ptr = NULL;   /* use the config bundled in bmi270.c */

    _rslt = bmi270_init(&_dev);
    if (_rslt != BMI2_OK) {
        /* One retry: init starts with a soft reset, so it recovers cleanly
         * from a transient bus error during the config upload. */
        delay(50);
        _rslt = bmi270_init(&_dev);
    }
    if (_rslt != BMI2_OK) return false;

    /* Accelerometer 100 Hz / ±4 g, gyroscope 200 Hz / ±2000 dps. */
    struct bmi2_sens_config cfg[2];
    cfg[0].type = BMI2_ACCEL;
    cfg[1].type = BMI2_GYRO;
    _rslt = bmi2_get_sensor_config(cfg, 2, &_dev);
    if (_rslt != BMI2_OK) return false;

    cfg[0].cfg.acc.odr = BMI2_ACC_ODR_100HZ;
    cfg[0].cfg.acc.range = BMI2_ACC_RANGE_4G;
    cfg[0].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;
    cfg[0].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;
    _accRangeG = 4.0f;

    cfg[1].cfg.gyr.odr = BMI2_GYR_ODR_200HZ;
    cfg[1].cfg.gyr.range = BMI2_GYR_RANGE_2000;
    cfg[1].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;
    cfg[1].cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE;
    cfg[1].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;
    _gyrRangeDps = 2000.0f;

    _rslt = bmi2_set_sensor_config(cfg, 2, &_dev);
    if (_rslt != BMI2_OK) return false;

    uint8_t sensors[2] = { BMI2_ACCEL, BMI2_GYRO };
    _rslt = bmi2_sensor_enable(sensors, 2, &_dev);
    if (_rslt != BMI2_OK) return false;

    _ready = true;
    return true;
}

bool SensairBMI270::read(SensairMotionData &data) {
    if (!_ready) return false;

    struct bmi2_sens_data raw;
    _rslt = bmi2_get_sensor_data(&raw, &_dev);
    if (_rslt != BMI2_OK) return false;

    /* Raw 16-bit two's complement -> physical units. */
    const float halfScale = (float)(1UL << (_dev.resolution - 1));
    data.ax = _accRangeG * raw.acc.x / halfScale;
    data.ay = _accRangeG * raw.acc.y / halfScale;
    data.az = _accRangeG * raw.acc.z / halfScale;
    data.gx = _gyrRangeDps * raw.gyr.x / halfScale;
    data.gy = _gyrRangeDps * raw.gyr.y / halfScale;
    data.gz = _gyrRangeDps * raw.gyr.z / halfScale;
    data.accValid = raw.status & BMI2_DRDY_ACC;
    data.gyrValid = raw.status & BMI2_DRDY_GYR;
    return true;
}

/* ------------------------------------------------------- Bosch API glue --- */

BMI2_INTF_RETURN_TYPE SensairBMI270::i2cRead(uint8_t reg, uint8_t *data, uint32_t len, void *intf) {
    I2cLink *l = (I2cLink *)intf;
    l->wire->beginTransmission(l->addr);
    l->wire->write(reg);
    if (l->wire->endTransmission(false) != 0) return BMI2_E_COM_FAIL;
    uint32_t got = 0;
    while (got < len) {
        uint8_t chunk = (uint8_t)min(len - got, (uint32_t)32);
        bool last = (got + chunk) == len;
        uint8_t n = l->wire->requestFrom(l->addr, chunk, (uint8_t)last);
        if (n == 0) return BMI2_E_COM_FAIL;
        while (n-- && got < len) data[got++] = l->wire->read();
    }
    return BMI2_OK;
}

BMI2_INTF_RETURN_TYPE SensairBMI270::i2cWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *intf) {
    I2cLink *l = (I2cLink *)intf;
    l->wire->beginTransmission(l->addr);
    l->wire->write(reg);
    l->wire->write(data, len);
    return l->wire->endTransmission() == 0 ? BMI2_OK : BMI2_E_COM_FAIL;
}

void SensairBMI270::delayUs(uint32_t us, void *) {
    if (us >= 1000) {
        delay(us / 1000);
        us %= 1000;
    }
    if (us) delayMicroseconds(us);
}
