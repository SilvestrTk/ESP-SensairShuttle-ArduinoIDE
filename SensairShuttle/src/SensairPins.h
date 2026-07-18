/*
 * SensairPins.h — ESP-SensairShuttle v1.0 pin map and I2C addresses
 *
 * Board: Espressif ESP-SensairShuttle (ESP32-C5-WROOM-1-N16R8)
 * Sources: ESP-SensairShuttle-MainBoard V1.0 schematic (Espressif, 2025-12-16)
 *          esp-dev-kits factory firmware board files
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_PINS_H
#define SENSAIR_PINS_H

/* ---------------------------------------------------------------------------
 * I2C bus (shared): touch panel, touch-button controller, Bosch shuttle board
 * 2.2 kOhm pull-ups on the 3.3 V side; PCA9306 level shifter to the
 * sensor rail (VDD_SENSOR, 1.8 V or 3.3 V selected by solder jumper H1).
 * ------------------------------------------------------------------------ */
#define SENSAIR_PIN_I2C_SDA        2
#define SENSAIR_PIN_I2C_SCL        3
#define SENSAIR_I2C_FREQ_DEFAULT   100000UL   /* factory firmware uses 100 kHz */

/* ---------------------------------------------------------------------------
 * 1.83" LCD, ST7789P3 controller, 240(H) x 284(V), 4-line SPI
 * No reset line (tied to LCD_3V3); reset by power-cycling PWR_CTRL.
 * Backlight is hardwired on (LEDK via 10 Ohm to GND).
 * ------------------------------------------------------------------------ */
#define SENSAIR_PIN_LCD_MOSI       23   /* LCD_SDA */
#define SENSAIR_PIN_LCD_SCLK       24   /* LCD_SCL */
#define SENSAIR_PIN_LCD_CS         25
#define SENSAIR_PIN_LCD_DC         26
#define SENSAIR_LCD_WIDTH          240  /* panel native (portrait) */
#define SENSAIR_LCD_HEIGHT         284
#define SENSAIR_LCD_SPI_FREQ       20000000UL  /* factory firmware: 20 MHz */

/* ---------------------------------------------------------------------------
 * Power control: gates LCD_3V3 and AUDIO_3V3 through a P-MOSFET.
 * LOW  = peripherals powered (default at boot: 10 MOhm pull-down)
 * HIGH = display + audio off
 * ------------------------------------------------------------------------ */
#define SENSAIR_PIN_PWR_CTRL       5

/* ---------------------------------------------------------------------------
 * Bosch Shuttle Board connector (9+7 pin, 1.27 mm, shuttle board 3.0)
 * Communication runs over the shared I2C bus above.
 *
 * WARNING: CS and SDO pass through a PCA9306 level shifter to the sensor's
 * 1.8 V rail. NEVER drive them HIGH at 3.3 V — that overdrives the sensor
 * pin through the shifter and the sensor drops off the I2C bus (verified on
 * hardware). Drive LOW, or set the pin to INPUT and let the on-board
 * pull-ups provide the high level.
 * ------------------------------------------------------------------------ */
#define SENSAIR_PIN_SHUTTLE_CS     10   /* BM_CS  — float (INPUT) = I2C mode */
#define SENSAIR_PIN_SHUTTLE_SDO    9    /* BM_SDO — LOW or float, never HIGH */
#define SENSAIR_PIN_SHUTTLE_G1     28   /* BM_G1  — sensor INT, shared with BOOT button! */
#define SENSAIR_PIN_SHUTTLE_G2     0    /* BM_G2  — sensor INT2 */

/* ---------------------------------------------------------------------------
 * Buttons / misc
 * ------------------------------------------------------------------------ */
#define SENSAIR_PIN_BOOT_BUTTON    28   /* LOW when pressed (also BM_G1)    */
#define SENSAIR_PIN_WS2812         27   /* external RGB strip connector     */
#define SENSAIR_PIN_EXT_IO1        4    /* 4-pin external header            */
#define SENSAIR_PIN_EXT_IO2        5    /* only if 0 Ohm R14 is populated   */

/* ---------------------------------------------------------------------------
 * Audio (not covered by this library yet — pins documented for reference)
 * Speaker: differential PDM direct to NS4150B class-D amplifier.
 * Microphone: analog electret through LMV321 op-amp into ADC.
 * ------------------------------------------------------------------------ */
#define SENSAIR_PIN_PA_ENABLE      1    /* NS4150B CTRL, HIGH = amp on      */
#define SENSAIR_PIN_PDM_P          7    /* speaker PDM +                    */
#define SENSAIR_PIN_PDM_N          8    /* speaker PDM -                    */
#define SENSAIR_PIN_MIC_ADC        6    /* OPA_OUT, ADC1 channel 5          */

/* ---------------------------------------------------------------------------
 * I2C device addresses
 * ------------------------------------------------------------------------ */
#define SENSAIR_ADDR_TOUCH         0x15 /* CST816S touch controller         */
#define SENSAIR_ADDR_TOUCH_BTN     0x50 /* BS8112 touch-button controller   */
#define SENSAIR_ADDR_BME690        0x76 /* SDO low (library default)        */
#define SENSAIR_ADDR_BME690_ALT    0x77 /* SDO floating (pulled high)       */
#define SENSAIR_ADDR_BMI270        0x68 /* SDO low (library default)        */
#define SENSAIR_ADDR_BMI270_ALT    0x69 /* SDO floating (pulled high)       */
#define SENSAIR_ADDR_BMM350        0x14 /* direct on main bus (shuttle V1.1+) */
#define SENSAIR_ADDR_BMM350_AUX    0x14 /* same chip via BMI270 auxiliary bus */

#endif /* SENSAIR_PINS_H */
