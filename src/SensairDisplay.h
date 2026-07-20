/*
 * SensairDisplay.h — driver for the ESP-SensairShuttle 1.83" LCD
 *
 * Panel:      1.83", 240(H) x 284(V), ST7789P3 controller, 4-line SPI
 * Wiring:     MOSI=GPIO23, SCLK=GPIO24, CS=GPIO25, DC=GPIO26
 *             No reset line — the panel is reset by power-cycling the
 *             peripheral rail (PWR_CTRL, GPIO5). Backlight is hardwired on.
 * Init:       vendor init sequence taken from the Espressif factory firmware.
 *
 * Self-contained: no Adafruit/TFT_eSPI dependency. Inherits Print, so
 * display.print()/println()/printf() work like Serial.
 *
 * Part of the SensairShuttle Arduino library.
 */
#ifndef SENSAIR_DISPLAY_H
#define SENSAIR_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>
#include "SensairPins.h"

/* A few ready-made RGB565 colors */
#define SENSAIR_BLACK   0x0000
#define SENSAIR_WHITE   0xFFFF
#define SENSAIR_RED     0xF800
#define SENSAIR_GREEN   0x07E0
#define SENSAIR_BLUE    0x001F
#define SENSAIR_YELLOW  0xFFE0
#define SENSAIR_CYAN    0x07FF
#define SENSAIR_MAGENTA 0xF81F
#define SENSAIR_ORANGE  0xFD20
#define SENSAIR_GREY    0x8410
#define SENSAIR_DARKGREY 0x39E7
#define SENSAIR_NAVY    0x000F

class SensairDisplay : public Print {
public:
    /*
     * Power the panel rail, start SPI and run the panel init sequence.
     * Returns true when the init sequence was sent.
     */
    bool begin(uint32_t spiFreq = SENSAIR_LCD_SPI_FREQ, SPIClass &spi = SPI);

    /*
     * 0 = portrait 240x284 (USB at the bottom)
     * 1 = landscape 284x240 (factory firmware orientation)
     * 2 = portrait flipped
     * 3 = landscape flipped
     */
    void setRotation(uint8_t r);
    uint8_t rotation() const { return _rot; }
    int16_t width() const  { return _width;  }
    int16_t height() const { return _height; }

    /* If the image appears shifted, trim the panel window here. */
    void setPanelOffset(int16_t colOffset, int16_t rowOffset);

    /* --- drawing ------------------------------------------------------- */
    void fillScreen(uint16_t color);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
    void fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);

    /* Raw pixel streaming (RGB565), for images or custom rendering. */
    void setAddrWindow(int16_t x, int16_t y, int16_t w, int16_t h);
    void pushColors(const uint16_t *colors, uint32_t count);

    /* --- text ---------------------------------------------------------- */
    void setCursor(int16_t x, int16_t y) { _cx = x; _cy = y; }
    void setTextColor(uint16_t fg)               { _fg = fg; _bg = fg; }  /* transparent bg */
    void setTextColor(uint16_t fg, uint16_t bg)  { _fg = fg; _bg = bg; }
    void setTextSize(uint8_t s) { _textSize = s ? s : 1; }
    void setTextWrap(bool wrap) { _wrap = wrap; }
    void drawChar(int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg, uint8_t size);
    using Print::write;                  /* keep the buffer overloads visible */
    size_t write(uint8_t c) override;    /* enables print()/printf() */

    /* Put the panel to sleep / wake it (backlight stays on — it is hardwired). */
    void sleep(bool on);

    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

private:
    void writeCommand(uint8_t cmd, const uint8_t *data = nullptr, size_t len = 0);
    void startWrite();
    void endWrite();
    void pushColorN(uint16_t color, uint32_t count); /* repeat one color */

    SPIClass *_spi = nullptr;
    SPISettings _spiSettings;
    uint8_t _rot = 0;
    int16_t _width = SENSAIR_LCD_WIDTH;
    int16_t _height = SENSAIR_LCD_HEIGHT;
    int16_t _xoff = 0, _yoff = 0;          /* current rotation offsets */
    int16_t _colTrim = 0, _rowTrim = 0;    /* user trim, see setPanelOffset */

    int16_t _cx = 0, _cy = 0;
    uint16_t _fg = SENSAIR_WHITE, _bg = SENSAIR_WHITE;
    uint8_t _textSize = 1;
    bool _wrap = true;
    bool _ready = false;
};

#endif /* SENSAIR_DISPLAY_H */
