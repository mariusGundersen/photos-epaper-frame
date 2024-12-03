#include "Epaper.h"

RGB palette[6] = {
    0x0000, // black
    0xFFFF, // white
    0xFFE0, // yellow
    0xF800, // red
    0x001F, // blue
    0x07E0, // green
};

Epaper::Epaper(uint8_t cs, uint8_t dc, uint8_t busy, uint8_t reset, uint16_t width, uint16_t height)
    : GFXcanvas16(width, height),
      _epd(EPD_7in3e(cs, dc, busy, reset, width, height)),
      _dither(FloydSteinberg(6, palette))
{
}

Epaper::~Epaper() {}

void Epaper::updateDisplay()
{
    _epd.init();
    _epd.draw([&](int x, int y)
              { return getPixel(x, y); });
    _epd.sleep();
}

void Epaper::dither()
{
    _dither.dither(_width, _height, getBuffer());
}

uint16_t Epaper::printCentredText(const char *buf, int x, int y, bool centerVertically)
{
    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds(buf, 0, 0, &x1, &y1, &w, &h); // calc width of new string

    setCursor(x - w / 2, y - (centerVertically ? h / 2 : 0) + (gfxFont ? h : 0));
    print(buf);
    return h;
}
