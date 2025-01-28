#include "Epaper.h"

Epaper::Epaper(uint8_t cs, uint8_t dc, uint8_t busy, uint8_t reset, uint16_t width, uint16_t height)
    : GFXcanvas16(width, height),
      _epd(EPD_7in3e(cs, dc, busy, reset)),
      _dither(FloydSteinberg())
{
}

Epaper::~Epaper() {}

void Epaper::updateDisplay()
{
    _epd.init();
    //_epd.clear(EPD_7IN3E_WHITE);
    _epd.draw(WIDTH, HEIGHT, [&](int x, int y)
              { 
                switch(getRawPixel(x, y)){
                    case RGB_BLACK:
                        return 0;
                    case RGB_WHITE:
                        return 1;
                    case RGB_YELLOW:
                        return 2;
                    case RGB_RED:
                        return 3;
                    case RGB_ORANGE:
                        return 4;
                    case RGB_BLUE:
                        return 5;
                    case RGB_GREEN:
                        return 6;
                    default: 
                        return 0;
                } });
    _epd.sleep();
}

void Epaper::dither()
{
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
    unsigned long lTime = micros();
    _dither.dither(WIDTH, HEIGHT, getBuffer());
    lTime = micros() - lTime;
    log_d("Dithered image in %d us\n", (int)lTime);
#else
    _dither.dither(WIDTH, HEIGHT, getBuffer());
#endif
}

uint16_t Epaper::printCentredText(const char *buf, bool centerVertically)
{
    return printCentredText(buf, _width / 2, _height / 2, centerVertically);
}

uint16_t Epaper::printCentredText(const char *buf, int y, bool centerVertically)
{
    return printCentredText(buf, _width / 2, y, centerVertically);
}

uint16_t Epaper::printCentredText(const char *buf, int x, int y, bool centerVertically)
{
    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds(buf, 0, 0, &x1, &y1, &w, &h); // calc width of new string

    x -= w / 2;
    y -= (centerVertically ? h / 2 : 0) - (gfxFont ? h : 0);
    setCursor(x, y);
    print(buf);
    return y + h;
}
