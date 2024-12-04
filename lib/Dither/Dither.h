

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
    RGB(uint16_t pixel)
    {
        r = (pixel >> 11) & RGB_RED_FULL;
        g = (pixel >> 5) & RGB_GREEN_FULL;
        b = (pixel >> 0) & RGB_BLUE_FULL;
    }
    RGB(uint8_t r, uint8_t g, uint8_t b)
    {
        this->r = clamp(r, 0, RGB_RED_FULL);
        this->g = clamp(g, 0, RGB_GREEN_FULL);
        this->b = clamp(b, 0, RGB_BLUE_FULL);
    }
    uint8_t r;
    uint8_t g;
    uint8_t b;
    operator uint16_t() const
    {
        return (uint16_t)(((r & RGB_RED_FULL) << 11) | ((g & RGB_GREEN_FULL) << 5) | (b & RGB_BLUE_FULL));
    }

    void add_error(int e[3], int q)
    {
        r = clamp(r + e[0] * q / 16, 0, RGB_RED_FULL);
        g = clamp(g + e[1] * q / 16, 0, RGB_GREEN_FULL);
        b = clamp(b + e[2] * q / 16, 0, RGB_BLUE_FULL);
    }
};

class FloydSteinberg
{
    uint8_t palette_size;
    RGB *palette;

public:
    FloydSteinberg(uint8_t palette_size, RGB *palette);
    uint16_t find_closest_palette_color(uint16_t pixel);
    uint16_t find_closest_palette_color(uint16_t pixel, int *errors, int evenOdd);
    void dither(const int w, const int h, uint16_t *pixels);
};
