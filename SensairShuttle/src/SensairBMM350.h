/*
 * SensairBMM350.h — BMM350 3-axis magnetometer (ShuttleBoard-BMI270&BMM350)
 *
 * Two access paths, depending on the shuttle board revision:
 *
 *   1. DIRECT (main I2C bus, address 0x14) — available on shuttle boards
 *      where the magnetometer is also wired to the main bus (confirmed on
 *      ShuttleBoard-BMI270&BMM350 V1.1). Does not need the BMI270:
 *          SensairBMM350 mag;
 *          mag.begin();                       // direct on Wire
 *
 *   2. AUX TUNNEL — through the BMI270's auxiliary I2C master, the way the
 *      Espressif factory firmware does it. Works on every revision, but
 *      requires an already-started SensairBMI270:
 *          SensairBMI270 imu;  imu.begin();
 *          mag.begin(imu);                    // tunneled through the IMU
 *
 * Simplest robust pattern (used by the examples): try direct first, fall
 * back to the aux tunnel if it fails.
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_BMM350_H
#define SENSAIR_BMM350_H

#include <Arduino.h>
#include <Wire.h>
#include "SensairPins.h"
#include "SensairBMI270.h"
#include "vendor/bmm350.h"

struct SensairMagData {
    float x = 0, y = 0, z = 0;  /* magnetic field, microtesla */
    float temperature = 0;      /* deg C, from the BMM350 */
};

class SensairBMM350 {
public:
    /*
     * Direct mode: talk to the BMM350 on the main I2C bus (address 0x14).
     * Returns false quickly when the magnetometer is not reachable there
     * (older shuttle revision) — then use the aux overload below.
     */
    bool begin(TwoWire &wire = Wire, uint32_t i2cFreq = SENSAIR_I2C_FREQ_DEFAULT,
               uint8_t addr = SENSAIR_ADDR_BMM350);

    /*
     * Aux mode: configure the BMI270 auxiliary interface and initialize the
     * BMM350 behind it. The passed SensairBMI270 must already have begin()
     * called successfully.
     */
    bool begin(SensairBMI270 &imu);

    /* Read a compensated magnetometer sample. */
    bool read(SensairMagData &data);

    /*
     * Compass heading in degrees (0 = magnetic north, 90 = east), assuming
     * the board is held level. No tilt compensation.
     */
    static float heading(const SensairMagData &d);

    /* Total field strength in microtesla (Earth field is ~25..65 uT). */
    static float fieldStrength(const SensairMagData &d);

    /*
     * Clear residual magnetization (Bosch "magnetic reset"). Called during
     * begin(); call again after exposure to a strong magnet.
     */
    bool magneticReset();

    /* Result code of the last Bosch API call (BMM350_OK == 0). */
    int8_t lastResult() const { return _rslt; }

    /* True when begin() used the aux tunnel, false for direct mode. */
    bool usingAux() const { return _bmi != nullptr; }

    /* Direct access to the Bosch device handle for advanced use. */
    struct bmm350_dev *dev() { return &_dev; }

private:
    bool finishInit();
    static int8_t auxRead(uint8_t reg, uint8_t *data, uint32_t len, void *intf);
    static int8_t auxWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *intf);
    static int8_t i2cRead(uint8_t reg, uint8_t *data, uint32_t len, void *intf);
    static int8_t i2cWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *intf);
    static void delayUs(uint32_t us, void *intf);

    struct I2cLink {
        TwoWire *wire;
        uint8_t addr;
    };
    I2cLink _link{&Wire, SENSAIR_ADDR_BMM350};
    struct bmi2_dev *_bmi = nullptr;
    struct bmm350_dev _dev{};
    int8_t _rslt = 0;
    bool _ready = false;
};

#endif /* SENSAIR_BMM350_H */
