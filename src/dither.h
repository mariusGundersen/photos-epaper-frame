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

#define rgb565(r, g, b) (uint16_t)(((r) << 11) | ((g) << 5) | (b))

class FloydSteinberg
{
    uint8_t palette_size;
    uint16_t *palette;

public:
    FloydSteinberg(uint8_t palette_size, uint16_t *palette);
    uint8_t find_closest_palette_color(uint16_t pixel);
    uint8_t find_closest_palette_color(uint16_t pixel, int *errors);
    void dither(const int w, const int h, uint16_t *pixels);
};

void split(uint16_t pixel, uint8_t *rgb);

FloydSteinberg::FloydSteinberg(uint8_t palette_size, uint16_t *palette)
{
    this->palette_size = palette_size;
    this->palette = palette;
}

void add_error(uint16_t *pixels, int index, int *errors, uint8_t q)
{
    uint8_t rgb[3];
    split(pixels[index], rgb);
    rgb[0] = constrain(rgb[0] + errors[0] * q / 16, 0, 0b11111);
    rgb[1] = constrain(rgb[1] + errors[1] * q / 16, 0, 0b111111);
    rgb[2] = constrain(rgb[2] + errors[2] * q / 16, 0, 0b11111);
    pixels[index] = rgb565(rgb[0], rgb[1], rgb[2]);
}

void FloydSteinberg::dither(const int w, const int h, uint16_t *pixels)
{
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            uint16_t oldpixel = pixels[y * w + x];
            int errors[3];
            uint16_t newpixel = this->find_closest_palette_color(oldpixel, errors);
            pixels[y * w + x] = newpixel;
            if (x + 1 < w)
            {
                add_error(pixels, (y + 0) * w + (x + 1), errors, 7);
            }

            if (y + 1 < h)
            {
                if (x - 1 >= 0)
                {
                    add_error(pixels, (y + 1) * w + (x - 1), errors, 3);
                }

                add_error(pixels, (y + 1) * w + (x + 0), errors, 5);

                if (x + 1 < w)
                {
                    add_error(pixels, (y + 1) * w + (x + 1), errors, 1);
                }
            }
        }
    }
}

/*
    565 color
    palette

    rrrrrggggggbbbbb

*/

uint8_t FloydSteinberg::find_closest_palette_color(uint16_t pixel)
{
    int errors[3];
    return this->find_closest_palette_color(pixel, errors);
}

uint8_t FloydSteinberg::find_closest_palette_color(uint16_t pixel, int *errors)
{
    int smallest_diff = 0xffffff;
    uint8_t best = 0;
    uint8_t sample[3];
    uint8_t option[3];
    split(pixel, sample);
    // Serial.printf("pixel %x, %x,%x,%x\n", pixel, sample[0], sample[1], sample[2]);
    for (int i = 0; i < this->palette_size; i++)
    {
        split(this->palette[i], option);
        // Serial.printf("sample %x, %x,%x,%x\n", this->palette[i], option[0], option[1], option[2]);
        int e0 = sample[0] - option[0];
        int e1 = sample[1] - option[1];
        int e2 = sample[2] - option[2];
        int diff = abs(e0) + abs(e1) / 2 + abs(e2);
        // Serial.printf("test %d, diff=%d, e: %d %d %d\n", i, diff, e0, e1, e2);
        if (diff < smallest_diff)
        {
            smallest_diff = diff;
            best = i;
            errors[0] = e0;
            errors[1] = e1;
            errors[2] = e2;
        }
    }

    return best;
}

void split(uint16_t pixel, uint8_t *rgb)
{
    rgb[0] = (pixel >> 11) & 0b11111;
    rgb[1] = (pixel >> 5) & 0b111111;
    rgb[2] = (pixel >> 0) & 0b11111;
}