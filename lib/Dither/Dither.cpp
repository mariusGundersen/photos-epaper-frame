#include "Dither.h"

FloydSteinberg::FloydSteinberg(uint8_t palette_size, RGB *palette)
{
    this->palette_size = palette_size;
    this->palette = palette;
}

void add_error(uint16_t *pixel, int r, int g, int b, uint8_t q)
{
    RGB rgb = RGB(*pixel);

    r = rgb.r + r * q / 16;
    g = rgb.g + g * q / 16;
    b = rgb.b + b * q / 16;

    if (r > RGB_RED_FULL || g > RGB_GREEN_FULL || b > RGB_BLUE_FULL)
    {
        int m = max(r, max(g >> 1, b));
        r = r * RGB_RED_FULL / m;
        g = g * RGB_GREEN_FULL / (m << 1);
        b = b * RGB_BLUE_FULL / m;
    }

    *pixel = RGB(r, g, b);
}

void FloydSteinberg::dither(const int w, const int h, uint16_t *pixels)
{
    int row = 0;
    for (int y = 0; y < h; y++, row += w)
    {
        int next_row = row + w;
        bool has_next_row = y + 1 < h;

        for (int x = 0; x < w; x++)
        {
            RGB oldpixel = pixels[row + x];
            uint16_t newpixel = this->find_closest_palette_color(oldpixel, y + x);
            RGB after = this->palette[newpixel];
            int r = oldpixel.r - after.r;
            int g = oldpixel.g - after.g;
            int b = oldpixel.b - after.b;
            pixels[row + x] = newpixel;
            if (x + 1 < w)
            {
                add_error(&pixels[row + (x + 1)], r, g, b, 7);
            }

            if (has_next_row)
            {
                if (x - 1 >= 0)
                {
                    add_error(&pixels[next_row + (x - 1)], r, g, b, 3);
                }

                add_error(&pixels[next_row + (x + 0)], r, g, b, 5);

                if (x + 1 < w)
                {
                    add_error(&pixels[next_row + (x + 1)], r, g, b, 1);
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
                // cyan, pick the highest of blue and green
                g = g >> 1;

                if (g > b)
                {
                    return 6; // green
                }
                else if (g == b)
                {
                    return evenOdd & 1 ? 5 : 6; // blue or green
                }
                else
                {
                    return 5; // blue
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
                // magenta, pick the highest of red and blue

                if (r > b)
                {
                    return 3; // red
                }
                else if (r == b)
                {
                    return evenOdd & 1 ? 5 : 3; // blue or red
                }
                else
                {
                    return 5; // blue
                }
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