/*
 * SensairDisplay.cpp — ST7789P3 240x284 SPI driver for the ESP-SensairShuttle
 *
 * The init sequence below is the panel vendor sequence used by the
 * Espressif factory firmware (esp-dev-kits, factory_demo/setup_device.c).
 *
 * Part of the SensairShuttle Arduino library.
 */
#include "SensairDisplay.h"
#include "SensairFont.h"

/* ST7789 commands used here */
#define ST_SWRESET 0x01
#define ST_SLPIN   0x10
#define ST_SLPOUT  0x11
#define ST_INVON   0x21
#define ST_DISPON  0x29
#define ST_CASET   0x2A
#define ST_RASET   0x2B
#define ST_RAMWR   0x2C
#define ST_MADCTL  0x36
#define ST_COLMOD  0x3A

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20

/* Panel window sits at RAM rows 0..283 of the 240x320 controller RAM. */
#define PANEL_RAM_ROWS 320

bool SensairDisplay::begin(uint32_t spiFreq, SPIClass &spi) {
    _spi = &spi;
    _spiSettings = SPISettings(spiFreq, MSBFIRST, SPI_MODE3);

    /* The LCD rail is gated by PWR_CTRL (LOW = on). The panel has no reset
     * pin, so a clean power-up is what resets it. */
    pinMode(SENSAIR_PIN_PWR_CTRL, OUTPUT);
    digitalWrite(SENSAIR_PIN_PWR_CTRL, LOW);
    delay(30);

    pinMode(SENSAIR_PIN_LCD_CS, OUTPUT);
    digitalWrite(SENSAIR_PIN_LCD_CS, HIGH);
    pinMode(SENSAIR_PIN_LCD_DC, OUTPUT);
    digitalWrite(SENSAIR_PIN_LCD_DC, HIGH);

    _spi->begin(SENSAIR_PIN_LCD_SCLK, -1 /* no MISO */, SENSAIR_PIN_LCD_MOSI, SENSAIR_PIN_LCD_CS);
    _ready = true;

    /* Software reset, then the vendor init sequence. */
    writeCommand(ST_SWRESET);
    delay(150);

    writeCommand(ST_SLPOUT);                                   /* Sleep out */
    delay(120);
    { uint8_t d[] = {0x00}; writeCommand(ST_MADCTL, d, 1); }   /* Memory access ctrl */
    { uint8_t d[] = {0x05}; writeCommand(ST_COLMOD, d, 1); }   /* 16-bit pixels */
    { uint8_t d[] = {0x0C, 0x0C, 0x00, 0x33, 0x33}; writeCommand(0xB2, d, 5); } /* Porch */
    { uint8_t d[] = {0x05}; writeCommand(0xB7, d, 1); }        /* Gate control */
    { uint8_t d[] = {0x21}; writeCommand(0xBB, d, 1); }        /* VCOM */
    { uint8_t d[] = {0x2C}; writeCommand(0xC0, d, 1); }        /* LCM control */
    { uint8_t d[] = {0x01}; writeCommand(0xC2, d, 1); }        /* VDV/VRH enable */
    { uint8_t d[] = {0x15}; writeCommand(0xC3, d, 1); }        /* VRH set */
    { uint8_t d[] = {0x0F}; writeCommand(0xC6, d, 1); }        /* Frame rate */
    { uint8_t d[] = {0xA7}; writeCommand(0xD0, d, 1); }        /* Power control 1 */
    { uint8_t d[] = {0xA4, 0xA1}; writeCommand(0xD0, d, 2); }  /* Power control 1 */
    { uint8_t d[] = {0xA1}; writeCommand(0xD6, d, 1); }        /* Gate out in sleep */
    { uint8_t d[] = {0xF0, 0x05, 0x0E, 0x08, 0x0A, 0x17, 0x39, 0x54,
                     0x4E, 0x37, 0x12, 0x12, 0x31, 0x37};
      writeCommand(0xE0, d, sizeof(d)); }                      /* Positive gamma */
    { uint8_t d[] = {0xF0, 0x10, 0x14, 0x0D, 0x0B, 0x05, 0x39, 0x44,
                     0x4D, 0x38, 0x14, 0x14, 0x2E, 0x35};
      writeCommand(0xE1, d, sizeof(d)); }                      /* Negative gamma */
    { uint8_t d[] = {0x23, 0x00, 0x00}; writeCommand(0xE4, d, 3); } /* Gate position */
    writeCommand(ST_INVON);                                    /* Inversion on */
    writeCommand(ST_DISPON);                                   /* Display on */
    delay(20);

    setRotation(0);
    fillScreen(SENSAIR_BLACK);
    return true;
}

void SensairDisplay::setRotation(uint8_t r) {
    _rot = r & 3;
    uint8_t mad = 0x00;
    switch (_rot) {
        case 0: /* portrait, panel native */
            mad = 0x00;
            _width = SENSAIR_LCD_WIDTH;  _height = SENSAIR_LCD_HEIGHT;
            _xoff = _colTrim;            _yoff = _rowTrim;
            break;
        case 1: /* landscape — same orientation as the factory firmware UI */
            mad = MADCTL_MV | MADCTL_MX;
            _width = SENSAIR_LCD_HEIGHT; _height = SENSAIR_LCD_WIDTH;
            _xoff = _rowTrim;            _yoff = _colTrim;
            break;
        case 2: /* portrait flipped: window moves to the far end of RAM */
            mad = MADCTL_MX | MADCTL_MY;
            _width = SENSAIR_LCD_WIDTH;  _height = SENSAIR_LCD_HEIGHT;
            _xoff = _colTrim;
            _yoff = (PANEL_RAM_ROWS - SENSAIR_LCD_HEIGHT) + _rowTrim;
            break;
        case 3: /* landscape flipped */
            mad = MADCTL_MV | MADCTL_MY;
            _width = SENSAIR_LCD_HEIGHT; _height = SENSAIR_LCD_WIDTH;
            _xoff = (PANEL_RAM_ROWS - SENSAIR_LCD_HEIGHT) + _rowTrim;
            _yoff = _colTrim;
            break;
    }
    writeCommand(ST_MADCTL, &mad, 1);
}

void SensairDisplay::setPanelOffset(int16_t colOffset, int16_t rowOffset) {
    _colTrim = colOffset;
    _rowTrim = rowOffset;
    setRotation(_rot); /* recompute active offsets */
}

/* ---------------------------------------------------------------- low level */

void SensairDisplay::startWrite() {
    _spi->beginTransaction(_spiSettings);
    digitalWrite(SENSAIR_PIN_LCD_CS, LOW);
}

void SensairDisplay::endWrite() {
    digitalWrite(SENSAIR_PIN_LCD_CS, HIGH);
    _spi->endTransaction();
}

void SensairDisplay::writeCommand(uint8_t cmd, const uint8_t *data, size_t len) {
    if (!_ready) return;
    startWrite();
    digitalWrite(SENSAIR_PIN_LCD_DC, LOW);
    _spi->write(cmd);
    digitalWrite(SENSAIR_PIN_LCD_DC, HIGH);
    if (data && len) {
        _spi->writeBytes(data, len);
    }
    endWrite();
}

void SensairDisplay::setAddrWindow(int16_t x, int16_t y, int16_t w, int16_t h) {
    uint16_t x0 = x + _xoff, x1 = x + w - 1 + _xoff;
    uint16_t y0 = y + _yoff, y1 = y + h - 1 + _yoff;
    uint8_t ca[] = { (uint8_t)(x0 >> 8), (uint8_t)x0, (uint8_t)(x1 >> 8), (uint8_t)x1 };
    uint8_t ra[] = { (uint8_t)(y0 >> 8), (uint8_t)y0, (uint8_t)(y1 >> 8), (uint8_t)y1 };
    writeCommand(ST_CASET, ca, 4);
    writeCommand(ST_RASET, ra, 4);
}

void SensairDisplay::pushColors(const uint16_t *colors, uint32_t count) {
    if (!_ready) return;
    startWrite();
    digitalWrite(SENSAIR_PIN_LCD_DC, LOW);
    _spi->write(ST_RAMWR);
    digitalWrite(SENSAIR_PIN_LCD_DC, HIGH);
    uint8_t buf[128];
    uint32_t i = 0;
    while (i < count) {
        size_t n = 0;
        while (n + 1 < sizeof(buf) && i < count) {
            buf[n++] = colors[i] >> 8;   /* big-endian on the wire */
            buf[n++] = colors[i] & 0xFF;
            i++;
        }
        _spi->writeBytes(buf, n);
    }
    endWrite();
}

void SensairDisplay::pushColorN(uint16_t color, uint32_t count) {
    startWrite();
    digitalWrite(SENSAIR_PIN_LCD_DC, LOW);
    _spi->write(ST_RAMWR);
    digitalWrite(SENSAIR_PIN_LCD_DC, HIGH);
    uint8_t hi = color >> 8, lo = color & 0xFF;
    uint8_t buf[256];
    size_t fill = min((uint32_t)(sizeof(buf) / 2), count) * 2;
    for (size_t n = 0; n < fill; n += 2) { buf[n] = hi; buf[n + 1] = lo; }
    uint32_t remaining = count * 2;
    while (remaining) {
        size_t n = min((uint32_t)sizeof(buf), remaining);
        _spi->writeBytes(buf, n);
        remaining -= n;
    }
    endWrite();
}

/* ------------------------------------------------------------------ drawing */

void SensairDisplay::fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

void SensairDisplay::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    /* clip to screen */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > _width)  w = _width - x;
    if (y + h > _height) h = _height - y;
    if (w <= 0 || h <= 0 || !_ready) return;
    setAddrWindow(x, y, w, h);
    pushColorN(color, (uint32_t)w * h);
}

void SensairDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || y < 0 || x >= _width || y >= _height || !_ready) return;
    setAddrWindow(x, y, 1, 1);
    pushColorN(color, 1);
}

void SensairDisplay::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    fillRect(x, y, w, 1, color);
}

void SensairDisplay::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    fillRect(x, y, 1, h, color);
}

void SensairDisplay::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
}

void SensairDisplay::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    /* Bresenham */
    int16_t dx = abs(x1 - x0), dy = -abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void SensairDisplay::drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    /* Bresenham / Zingl midpoint circle */
    int16_t x = -r, y = 0, err = 2 - 2 * r;
    do {
        drawPixel(cx - x, cy + y, color);
        drawPixel(cx - y, cy - x, color);
        drawPixel(cx + x, cy - y, color);
        drawPixel(cx + y, cy + x, color);
        int16_t e2 = err;
        if (e2 <= y) { err += ++y * 2 + 1; }
        if (e2 > x || err > y) { err += ++x * 2 + 1; }
    } while (x < 0);
}

void SensairDisplay::fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    for (int16_t y = -r; y <= r; y++) {
        int16_t half = (int16_t)sqrtf((float)(r * r - y * y));
        drawFastHLine(cx - half, cy + y, 2 * half + 1, color);
    }
}

/* --------------------------------------------------------------------- text */

void SensairDisplay::drawChar(int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg, uint8_t size) {
    if (c < SENSAIR_FONT_FIRST_CHAR || c > SENSAIR_FONT_LAST_CHAR) c = '?';
    const uint8_t *glyph = &SENSAIR_FONT_5X7[(c - SENSAIR_FONT_FIRST_CHAR) * SENSAIR_FONT_W];
    bool opaque = (fg != bg);
    for (uint8_t col = 0; col < SENSAIR_FONT_W + 1; col++) {  /* +1 = spacing column */
        uint8_t bits = (col < SENSAIR_FONT_W) ? pgm_read_byte(&glyph[col]) : 0x00;
        for (uint8_t row = 0; row < SENSAIR_FONT_H + 1; row++) { /* +1 = line spacing */
            bool set = (row < SENSAIR_FONT_H) && (bits & (1u << row));
            if (set || opaque) {
                uint16_t color = set ? fg : bg;
                if (!set && !opaque) continue;
                if (size == 1) {
                    drawPixel(x + col, y + row, color);
                } else {
                    fillRect(x + col * size, y + row * size, size, size, color);
                }
            }
        }
    }
}

size_t SensairDisplay::write(uint8_t c) {
    int16_t adv = (SENSAIR_FONT_W + 1) * _textSize;
    int16_t lineH = (SENSAIR_FONT_H + 1) * _textSize;
    if (c == '\n') {
        _cx = 0;
        _cy += lineH;
        return 1;
    }
    if (c == '\r') return 1;
    if (_wrap && (_cx + adv) > _width) {
        _cx = 0;
        _cy += lineH;
    }
    drawChar(_cx, _cy, (char)c, _fg, _bg, _textSize);
    _cx += adv;
    return 1;
}

void SensairDisplay::sleep(bool on) {
    writeCommand(on ? ST_SLPIN : ST_SLPOUT);
    delay(on ? 10 : 120);
}
