

/*
for each y from top to bottom do
    for each x from left to right do
        oldpixel := pixels[x][y]
        newpixel := find_closest_palette_color(oldpixel)
        pixels[x][y] := newpixel
        quant_error := oldpixel - newpixel
        pixels[x + 1][y    ] := pixels[x + 1][y    ] + quant_error × 7 / 16
        pixels[x - 1][y + 1] := pixels[x - 1][y + 1] + quant_error × 3 / 16
        pixels[x    ][y + 1] := pixels[x    ][y + 1] + quant_error × 5 / 16
        pixels[x + 1][y + 1] := pixels[x + 1][y + 1] + quant_error × 1 / 16
*/
#include <cstdint>
#include <stdlib.h>
#include <Arduino.h>

int clamp(int value, int min, int max);

#define RGB_RED_FULL 0b11111
#define RGB_GREEN_FULL 0b111111
#define RGB_BLUE_FULL 0b11111

#define RGB_RED_HALF 0b01111
#define RGB_GREEN_HALF 0b011111
#define RGB_BLUE_HALF 0b01111

struct RGB
{
    RGB(uint16_t pixel) : r((pixel >> 11)),
                          g((pixel >> 5)),
                          b((pixel >> 0))
    {
    }

    RGB(uint8_t r, uint8_t g, uint8_t b) : r(r),
                                           g(g),
                                           b(b)
    {
    }

    const uint16_t r : 5;
    const uint16_t g : 6;
    const uint16_t b : 5;

    operator uint16_t() const
    {
        return (uint16_t)((r << 11) | (g << 5) | b);
    }
};

class FloydSteinberg
{
    uint8_t palette_size;
    RGB *palette;

public:
    FloydSteinberg(uint8_t palette_size, RGB *palette);
    uint16_t find_closest_palette_color(uint16_t pixel, int evenOdd);
    void dither(const int w, const int h, uint16_t *pixels);
};
