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

void get_errors(int *errors, RGB before, RGB after)
{
    errors[0] = before.r - after.r;
    errors[1] = before.g - after.g;
    errors[2] = before.b - after.b;
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
            uint16_t newpixel = this->find_closest_palette_color(oldpixel, y + x);
            get_errors(errors, oldpixel, this->palette[newpixel]);
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
    return this->find_closest_palette_color(pixel, 0);
}

uint16_t FloydSteinberg::find_closest_palette_color(uint16_t pixel, int evenOdd)
{
    RGB sample = RGB(pixel);
    uint8_t r = sample.r;
    uint8_t g = sample.g;
    uint8_t b = sample.b;

    if (r < RGB_RED_HALF)
    {
        if (g < RGB_GREEN_HALF)
        {
            if (b < RGB_BLUE_HALF)
            {
                // black
                return 0;
            }
            else
            {
                // blue
                return 5;
            }
        }
        else
        {
            if (b < RGB_BLUE_HALF)
            {
                // green
                return 6;
            }
            else
            {
                // cyan, pick the highest of blue and green and white
                uint8_t w = RGB_RED_FULL - r;
                g = g >> 1;

                if (g > b)
                {
                    if (w > g)
                    {
                        return 1; // white
                    }
                    else if (w == g)
                    {
                        return evenOdd & 1 ? 1 : 6; // white or green
                    }
                    else
                    {
                        return 6; // green
                    }
                }
                else if (g == b)
                {
                    if (w > g)
                    {
                        return 1; // white;
                    }
                    else if (w == g)
                    {
                        switch (evenOdd % 3)
                        {
                        case 0:
                            return 1; // white;
                        case 1:
                            return 5; // blue;
                        case 2:
                        default:
                            return 6; // green;
                        }
                    }
                    else
                    {
                        return evenOdd & 1 ? 5 : 6; // blue or green
                    }
                }
                else
                {
                    if (w > b)
                    {
                        return 1; // white
                    }
                    else if (w == b)
                    {
                        return evenOdd & 1 ? 1 : 5; // white or blue
                    }
                    else
                    {
                        return 5; // blue
                    }
                }
            }
        }
    }
    else
    {
        if (g < RGB_GREEN_HALF)
        {
            if (b < RGB_BLUE_HALF)
            {
                // red
                return 3;
            }
            else
            {
                // magenta, pick the highest of red, blue and white

                uint8_t w = (RGB_GREEN_FULL - g) >> 1;

                if (r > b)
                {
                    if (r > w)
                    {
                        return 3; // red
                    }
                    else if (w == r)
                    {
                        return evenOdd & 1 ? 1 : 3; // white or red
                    }
                    else
                    {
                        return 1; // white
                    }
                }
                else if (r == b)
                {
                    if (w > r)
                    {
                        return 1; // white;
                    }
                    else if (w == r)
                    {
                        switch (evenOdd % 3)
                        {
                        case 0:
                            return 1; // white;
                        case 1:
                            return 5; // blue;
                        case 2:
                        default:
                            return 3; // red;
                        }
                    }
                    else
                    {
                        return evenOdd & 1 ? 5 : 3; // blue or red
                    }
                }
                else
                {
                    if (b > w)
                    {
                        return 5; // blue
                    }
                    else if (w == b)
                    {
                        return evenOdd & 1 ? 1 : 5; // white or blue
                    }
                    else
                    {
                        return 1; // white
                    }
                }

                bool redish = (evenOdd & 1) ? r > b : r >= b;
                return redish ? 3 : 5;
            }
        }
        else
        {
            if (b < RGB_BLUE_HALF)
            {
                // yellow
                return 2;
            }
            else
            {
                // white
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