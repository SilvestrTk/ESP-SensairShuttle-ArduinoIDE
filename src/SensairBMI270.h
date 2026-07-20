/*
 * SensairBMI270.h — BMI270 6-axis IMU (ShuttleBoard-BMI270&BMM350)
 *
 * Accelerometer + gyroscope, using the official Bosch BMI270 SensorAPI
 * (bundled in src/vendor) over I2C. begin() uploads the ~8 KB Bosch
 * configuration blob, so it takes a moment.
 *
 * Wiring on the ESP-SensairShuttle: shared I2C bus (SDA=GPIO2, SCL=GPIO3),
 * address select pin BM_SDO=GPIO9 (driven LOW -> address 0x68, left
 * floating -> 0x69). BM_CS=GPIO10 is left floating: on-board pull-ups keep
 * it high (I2C mode). The select lines must never be driven HIGH — see the
 * warning in SensairPins.h.
 *
 * The BMM350 magnetometer on the same shuttle board hangs off this chip's
 * auxiliary interface — see SensairBMM350, which requires a started
 * SensairBMI270.
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_BMI270_H
#define SENSAIR_BMI270_H

#include <Arduino.h>
#include <Wire.h>
#include "SensairPins.h"
#include "vendor/bmi270.h"

struct SensairMotionData {
    float ax = 0, ay = 0, az = 0;   /* acceleration, g */
    float gx = 0, gy = 0, gz = 0;   /* angular rate, deg/s */
    bool accValid = false;          /* data-ready flags from the sensor */
    bool gyrValid = false;
};

class SensairBMI270 {
public:
    /*
     * Initializes the IMU (config upload + accel 100 Hz / ±4 g,
     * gyro 200 Hz / ±2000 dps). Returns true on success.
     */
    bool begin(TwoWire &wire = Wire, uint32_t i2cFreq = SENSAIR_I2C_FREQ_DEFAULT,
               uint8_t addr = SENSAIR_ADDR_BMI270);

    /* Read the latest accelerometer + gyroscope sample. */
    bool read(SensairMotionData &data);

    /* Result code of the last Bosch API call (BMI2_OK == 0). */
    int8_t lastResult() const { return _rslt; }

    bool isReady() const { return _ready; }

    /* Bosch device handle — needed by SensairBMM350 and for advanced use
     * (interrupts, step counter, gesture features, FIFO, ...). */
    struct bmi2_dev *dev() { return &_dev; }

    /* Ranges used for the raw -> unit conversion. */
    float accelRangeG() const { return _accRangeG; }
    float gyroRangeDps() const { return _gyrRangeDps; }

private:
    struct I2cLink {
        TwoWire *wire;
        uint8_t addr;
    };
    static BMI2_INTF_RETURN_TYPE i2cRead(uint8_t reg, uint8_t *data, uint32_t len, void *intf);
    static BMI2_INTF_RETURN_TYPE i2cWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *intf);
    static void delayUs(uint32_t us, void *intf);

    I2cLink _link{&Wire, SENSAIR_ADDR_BMI270};
    struct bmi2_dev _dev{};
    int8_t _rslt = 0;
    bool _ready = false;
    float _accRangeG = 4.0f;
    float _gyrRangeDps = 2000.0f;
};

#endif /* SENSAIR_BMI270_H */
