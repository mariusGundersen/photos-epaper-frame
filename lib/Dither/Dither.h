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

struct RGB
{
    RGB(uint16_t pixel)
    {
        r = (pixel >> 11) & 0b11111;
        g = (pixel >> 5) & 0b111111;
        b = (pixel >> 0) & 0b11111;
    }
    RGB(uint8_t r, uint8_t g, uint8_t b)
    {
        this->r = clamp(r, 0, 0x1f);
        this->g = clamp(g, 0, 0x3f);
        this->b = clamp(b, 0, 0x1f);
    }
    uint8_t r;
    uint8_t g;
    uint8_t b;
    operator uint16_t() const
    {
        return (uint16_t)(((r) << 11) | ((g) << 5) | (b));
    }

    void add_error(int e[3], int q)
    {
        r = clamp(r + e[0] * q / 16, 0, 0b11111);
        g = clamp(g + e[1] * q / 16, 0, 0b111111);
        b = clamp(b + e[2] * q / 16, 0, 0b11111);
    }
};

class FloydSteinberg
{
    uint8_t palette_size;
    RGB *palette;

public:
    FloydSteinberg(uint8_t palette_size, RGB *palette);
    uint8_t find_closest_palette_color(uint16_t pixel);
    uint8_t find_closest_palette_color(uint16_t pixel, int *errors);
    void dither(const int w, const int h, uint16_t *pixels);
};
