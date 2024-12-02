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
    int smallest_diff = 0xffffff;
    uint8_t best = 0;
    RGB sample = RGB(pixel);
    // Serial.printf("pixel %x, %x,%x,%x\n", pixel, sample[0], sample[1], sample[2]);
    for (int i = 0; i < this->palette_size; i++)
    {
        RGB option = this->palette[i];
        // Serial.printf("sample %x, %x,%x,%x\n", this->palette[i], option[0], option[1], option[2]);
        int e0 = sample.r - option.r;
        int e1 = sample.g - option.g;
        int e2 = sample.b - option.b;
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

int clamp(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}