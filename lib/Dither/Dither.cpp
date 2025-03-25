#include "Dither.h"

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
            RGB newpixel = this->find_closest_palette_color(oldpixel, y + x);
            int r = oldpixel.r - newpixel.r;
            int g = oldpixel.g - newpixel.g;
            int b = oldpixel.b - newpixel.b;
            pixels[row + x] = newpixel;

            if (has_next_row)
            {
                add_error(&pixels[row + (x + 1)], r, g, b, 7);

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
            else if (x + 1 < w)
            {
                add_error(&pixels[row + (x + 1)], r, g, b, 7);
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
                return RGB_BLACK;
            }
            else
            {
                return RGB_BLUE;
            }
        }
        else
        {
            if (b < RGB_BLUE_HALF)
            {
                return RGB_GREEN;
            }
            else
            {
                // cyan, pick the highest of blue and green
                g = g >> 1;

                if (g > b)
                {
                    return RGB_GREEN;
                }
                else if (g == b)
                {
                    return evenOdd & 1 ? RGB_BLUE : RGB_GREEN;
                }
                else
                {
                    return RGB_BLUE;
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
                return RGB_RED;
            }
            else
            {
                // magenta, pick the highest of red and blue
                if (r > b)
                {
                    return RGB_RED; // red
                }
                else if (r == b)
                {
                    return evenOdd & 1 ? RGB_BLUE : RGB_RED;
                }
                else
                {
                    return RGB_BLUE;
                }
            }
        }
        else
        {
            if (b < RGB_BLUE_HALF)
            {
                return RGB_YELLOW;
            }
            else
            {
                return RGB_WHITE;
            }
        }
    }
}
