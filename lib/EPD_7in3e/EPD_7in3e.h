/*****************************************************************************
* | File        :   EPD_7in3e.h
* | Author      :   Waveshare team
* | Function    :   7.3inch e-Paper (F) Driver
* | Info        :
*----------------
* | This version:   V1.0
* | Date        :   2022-10-20
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#ifndef __EPD_7IN3E_H_
#define __EPD_7IN3E_H_

#include <Arduino.h>

// Display resolution
#define EPD_7IN3E_WIDTH 800
#define EPD_7IN3E_HEIGHT 480

/**********************************
Color Index
**********************************/
#define EPD_7IN3E_BLACK 0x0  /// 000
#define EPD_7IN3E_WHITE 0x1  /// 001
#define EPD_7IN3E_YELLOW 0x2 /// 010
#define EPD_7IN3E_RED 0x3    /// 011
// #define EPD_7IN3E_ORANGE  0x4   /// 100
#define EPD_7IN3E_BLUE 0x5  /// 101
#define EPD_7IN3E_GREEN 0x6 /// 110

class EPD_7in3e
{
    uint8_t _cs;
    uint8_t _dc;
    uint8_t _busy;
    uint8_t _reset;
    uint16_t _width;
    uint16_t _height;

    void reset(void);
    void sendCommand(uint8_t Reg);
    void sendData(uint8_t Data);
    void sendData(uint8_t *pData, uint32_t len);
    void readBusyH(void);
    void turnOnDisplay(void);

public:
    EPD_7in3e(uint8_t cs, uint8_t dc, uint8_t busy, uint8_t reset, uint16_t width, uint16_t height);
    void init();
    void clear(uint8_t color);
    void draw(std::function<uint8_t(int, int)> lambda);
    void sleep(void);
};

#endif