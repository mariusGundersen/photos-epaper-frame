#ifndef __EPAPER_H_
#define __EPAPER_H_

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <EPD_7in3e.h>
#include <Dither.h>

class Epaper : public GFXcanvas16
{
private:
    EPD_7in3e _epd;
    FloydSteinberg _dither;

public:
    Epaper(uint8_t cs, uint8_t dc, uint8_t busy, uint8_t reset, uint16_t width, uint16_t height);
    ~Epaper();
    void updateDisplay();
    void dither();

    uint16_t printCentredText(const char *buf, bool centerVertically = true);
    uint16_t printCentredText(const char *buf, int x, int y, bool centerVertically = true);
    uint16_t printCentredText(const char *buf, int y, bool centerVertically = true);
};

#endif