#include "Epaper.h"

RGB palette[7] = {
    0x0000, // black
    0xFFFF, // white
    0xFFE0, // yellow
    0xF800, // red
    0xFF80, // orange - unused
    0x001F, // blue
    0x07E0, // green
};

Epaper::Epaper(uint8_t cs, uint8_t dc, uint8_t busy, uint8_t reset, uint16_t width, uint16_t height)
    : GFXcanvas16(width, height),
      _epd(EPD_7in3e(cs, dc, busy, reset)),
      _dither(FloydSteinberg(7, palette))
{
}

Epaper::~Epaper() {}

void Epaper::updateDisplay()
{
    _epd.init();
    //_epd.clear(EPD_7IN3E_WHITE);
    _epd.draw(WIDTH, HEIGHT, [&](int x, int y)
              { return getRawPixel(x, y); });
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
