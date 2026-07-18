/*
 * SensairBME690.h — BME690 environmental sensor (ShuttleBoard-BME690)
 *
 * Measures temperature, relative humidity, barometric pressure and gas
 * resistance (air quality indicator). Uses the official Bosch BME69x
 * SensorAPI (bundled in src/vendor) in forced mode over I2C.
 *
 * Wiring on the ESP-SensairShuttle: shared I2C bus (SDA=GPIO2, SCL=GPIO3),
 * address select pin BM_SDO=GPIO9 (driven LOW -> address 0x76, left
 * floating -> 0x77). BM_CS=GPIO10 is left floating: on-board pull-ups keep
 * it high (I2C mode). The select lines must never be driven HIGH — see the
 * warning in SensairPins.h.
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_BME690_H
#define SENSAIR_BME690_H

#include <Arduino.h>
#include <Wire.h>
#include "SensairPins.h"
#include "vendor/bme69x.h"

struct SensairEnvData {
    float temperature   = 0;   /* deg C */
    float humidity      = 0;   /* % RH */
    float pressure      = 0;   /* hPa */
    float gasResistance = 0;   /* Ohm — higher generally means cleaner air */
    bool  gasValid      = false;
    bool  heaterStable  = false;
};

class SensairBME690 {
public:
    /*
     * Configures the shuttle select pins, initializes the sensor and applies
     * a default configuration (T x2 / P x16 / H x1 oversampling, gas heater
     * 300 degC for 100 ms). Returns true on success.
     */
    bool begin(TwoWire &wire = Wire, uint32_t i2cFreq = SENSAIR_I2C_FREQ_DEFAULT,
               uint8_t addr = SENSAIR_ADDR_BME690);

    /*
     * Runs one forced-mode measurement (blocks for roughly 150 ms with the
     * default configuration) and fills `data`. Returns true on success.
     */
    bool read(SensairEnvData &data);

    /*
     * Change the gas heater setpoint. Pass durationMs = 0 to disable the
     * heater entirely (faster, lower power, no gas reading).
     */
    bool setHeater(uint16_t tempC, uint16_t durationMs);

    /* Result code of the last Bosch API call (BME69X_OK == 0). */
    int8_t lastResult() const { return _rslt; }

    /* Direct access to the Bosch device handle for advanced use. */
    struct bme69x_dev *dev() { return &_dev; }

private:
    struct I2cLink {
        TwoWire *wire;
        uint8_t addr;
    };
    static BME69X_INTF_RET_TYPE i2cRead(uint8_t reg, uint8_t *data, uint32_t len, void *intf);
    static BME69X_INTF_RET_TYPE i2cWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *intf);
    static void delayUs(uint32_t us, void *intf);

    I2cLink _link{&Wire, SENSAIR_ADDR_BME690};
    struct bme69x_dev _dev{};
    struct bme69x_conf _conf{};
    struct bme69x_heatr_conf _heatr{};
    int8_t _rslt = 0;
    bool _ready = false;
};

#endif /* SENSAIR_BME690_H */
