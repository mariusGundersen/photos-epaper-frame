#include "Dither.h"

FloydSteinberg::FloydSteinberg(uint8_t palette_size, RGB *palette)
{
    this->palette_size = palette_size;
    this->palette = palette;
}

void add_error(uint16_t *pixel, int errors[3], uint8_t q)
{
    RGB rgb = RGB(*pixel);
    rgb.add_error(errors, q);
    *pixel = rgb;
}

void FloydSteinberg::dither(const int w, const int h, uint16_t *pixels)
{
    int errors[3];
    int row = 0;
    for (int y = 0; y < h; y++, row += w)
    {
        int next_row = row + w;
        bool has_next_row = y + 1 < h;

        for (int x = 0; x < w; x++)
        {
            uint16_t oldpixel = pixels[row + x];
            uint16_t newpixel = this->find_closest_palette_color(oldpixel, errors);
            pixels[row + x] = newpixel;
            if (x + 1 < w)
            {
                add_error(&pixels[row + (x + 1)], errors, 7);
            }

            if (has_next_row)
            {
                if (x - 1 >= 0)
                {
                    add_error(&pixels[next_row + (x - 1)], errors, 3);
                }

                add_error(&pixels[next_row + (x + 0)], errors, 5);

                if (x + 1 < w)
                {
                    add_error(&pixels[next_row + (x + 1)], errors, 1);
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
    RGB sample = RGB(pixel);

    if (sample.r < 0xf)
    {
        if (sample.g < 0x1f)
        {
            if (sample.b < 0xf)
            {
                // black
                errors[0] = sample.r;
                errors[1] = sample.g;
                errors[2] = sample.b;
                return 0;
            }
            else
            {
                // blue
                errors[0] = sample.r;
                errors[1] = sample.g;
                errors[2] = sample.b - 0x1f;
                return 4;
            }
        }
        else
        {
            if (sample.b < 0xf)
            {
                // green
                errors[0] = sample.r;
                errors[1] = sample.g - 0x3f;
                errors[2] = sample.b;
                return 5;
            }
            else
            {
                // cyan, pick the highest of blue and green
                errors[0] = sample.r;
                errors[1] = sample.g - (sample.g > sample.b ? 0x3f : 0);
                errors[2] = sample.b - (sample.g > sample.b ? 0 : 0x1f);
                return sample.g > sample.b ? 5 : 4;
            }
        }
    }
    else
    {
        if (sample.g < 0x1f)
        {
            if (sample.b < 0xf)
            {
                // red
                errors[0] = sample.r - 0x1f;
                errors[1] = sample.g;
                errors[2] = sample.b;
                return 3;
            }
            else
            {
                // magenta, pick the highest of red and blue
                errors[0] = sample.r - (sample.r > sample.b ? 0x1f : 0);
                errors[1] = sample.g;
                errors[2] = sample.b - (sample.r > sample.b ? 0 : 0x1f);
                return sample.r > sample.b ? 3 : 4;
            }
        }
        else
        {
            if (sample.b < 0xf)
            {
                // yellow
                errors[0] = sample.r - 0x1f;
                errors[1] = sample.g - 0x3f;
                errors[2] = sample.b;
                return 2;
            }
            else
            {
                // white
                errors[0] = sample.r - 0x1f;
                errors[1] = sample.g - 0x3f;
                errors[2] = sample.b - 0x1f;
                return 1;
            }
        }
    }
}

int clamp(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}