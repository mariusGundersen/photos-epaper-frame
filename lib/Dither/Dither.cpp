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
            uint16_t newpixel = this->find_closest_palette_color(oldpixel, errors, y + x);
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

uint16_t FloydSteinberg::find_closest_palette_color(uint16_t pixel)
{
    int errors[3];
    return this->find_closest_palette_color(pixel, errors, 0);
}

uint16_t FloydSteinberg::find_closest_palette_color(uint16_t pixel, int *errors, int evenOdd)
{
    RGB sample = RGB(pixel);

    if (sample.r < RGB_RED_HALF)
    {
        if (sample.g < RGB_GREEN_HALF)
        {
            if (sample.b < RGB_BLUE_HALF)
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
                errors[2] = sample.b - RGB_BLUE_FULL;
                return 5;
            }
        }
        else
        {
            if (sample.b < RGB_BLUE_HALF)
            {
                // green
                errors[0] = sample.r;
                errors[1] = sample.g - RGB_GREEN_FULL;
                errors[2] = sample.b;
                return 6;
            }
            else
            {
                // cyan, pick the highest of blue and green
                bool greenish = (evenOdd & 1) ? (sample.g >> 1) > sample.b : (sample.g >> 1) >= sample.b;
                errors[0] = sample.r;
                errors[1] = sample.g - (greenish ? RGB_GREEN_FULL : 0);
                errors[2] = sample.b - (greenish ? 0 : RGB_BLUE_FULL);
                return greenish ? 6 : 5;
            }
        }
    }
    else
    {
        if (sample.g < RGB_GREEN_HALF)
        {
            if (sample.b < RGB_BLUE_HALF)
            {
                // red
                errors[0] = sample.r - RGB_RED_FULL;
                errors[1] = sample.g;
                errors[2] = sample.b;
                return 3;
            }
            else
            {
                // magenta, pick the highest of red and blue
                bool redish = (evenOdd & 1) ? sample.r > sample.b : sample.r >= sample.b;
                errors[0] = sample.r - (redish ? RGB_RED_FULL : 0);
                errors[1] = sample.g;
                errors[2] = sample.b - (redish ? 0 : RGB_BLUE_FULL);
                return redish ? 3 : 5;
            }
        }
        else
        {
            if (sample.b < RGB_BLUE_HALF)
            {
                // yellow
                errors[0] = sample.r - RGB_RED_FULL;
                errors[1] = sample.g - RGB_GREEN_FULL;
                errors[2] = sample.b;
                return 2;
            }
            else
            {
                // white
                errors[0] = sample.r - RGB_RED_FULL;
                errors[1] = sample.g - RGB_GREEN_FULL;
                errors[2] = sample.b - RGB_BLUE_FULL;
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