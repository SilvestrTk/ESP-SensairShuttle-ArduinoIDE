/*
 * SensairShuttle.h — umbrella header for the SensairShuttle Arduino library
 *
 * Library for the Espressif ESP-SensairShuttle development board
 * (ESP32-C5 + 1.83" ST7789P3 touch LCD + Bosch Sensortec shuttle boards).
 *
 * Modules (each usable on its own — include just the header you need):
 *   SensairBoard    — power rail, shared I2C, BOOT button, I2C scanner
 *   SensairDisplay  — 1.83" 240x284 LCD (ST7789P3, SPI)
 *   SensairTouch    — CST816S capacitive touch (polling + gestures)
 *   SensairBME690   — environmental sensor shuttle board (T/RH/p/gas)
 *   SensairBMI270   — 6-axis IMU on the motion shuttle board
 *   SensairBMM350   — magnetometer on the motion shuttle board
 *                     (behind the BMI270 aux interface)
 *
 * Requires: arduino-esp32 core 3.3.0 or newer (ESP32-C5 support),
 *           board "ESP32C5 Dev Module".
 */
#ifndef SENSAIR_SHUTTLE_H
#define SENSAIR_SHUTTLE_H

#define SENSAIR_SHUTTLE_VERSION "1.0.0"

#include "SensairPins.h"
#include "SensairBoard.h"
#include "SensairDisplay.h"
#include "SensairTouch.h"
#include "SensairBME690.h"
#include "SensairBMI270.h"
#include "SensairBMM350.h"

#endif /* SENSAIR_SHUTTLE_H */
