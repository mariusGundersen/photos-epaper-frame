/*****************************************************************************
* | File        :   EPD_7in3e.c
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
#include "EPD_7in3e.h"
#include <SPI.h>

/******************************************************************************
function :  Software reset
parameter:
******************************************************************************/
void EPD_7in3e::reset(void)
{
    digitalWrite(_reset, 1);
    delay(20);
    digitalWrite(_reset, 0);
    delay(2);
    digitalWrite(_reset, 1);
    delay(20);
}

/******************************************************************************
function :  send command
parameter:
     Reg : Command register
******************************************************************************/
void EPD_7in3e::sendCommand(uint8_t Reg)
{
    digitalWrite(_dc, 0);
    digitalWrite(_cs, 0);
    SPI.write(Reg);
    digitalWrite(_cs, 1);
}

/******************************************************************************
function :  send data
parameter:
    Data : Write data
******************************************************************************/
void EPD_7in3e::sendData(uint8_t Data)
{
    digitalWrite(_dc, 1);
    digitalWrite(_cs, 0);
    SPI.write(Data);
    digitalWrite(_cs, 1);
}

/******************************************************************************
function :  send data
parameter:
    Data : Write data
******************************************************************************/
void EPD_7in3e::sendData(uint8_t *pData, uint32_t len)
{
    digitalWrite(_dc, 1);
    digitalWrite(_cs, 0);
    SPI.writeBytes(pData, len);
    digitalWrite(_cs, 1);
}

/******************************************************************************
function :  Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_7in3e::readBusyH(void)
{
    log_d("e-Paper busy H\r\n");
    while (!digitalRead(_busy))
    { // LOW: busy, HIGH: idle
        delay(1);
    }
    log_d("e-Paper busy H release\r\n");
}

EPD_7in3e::EPD_7in3e(uint8_t cs, uint8_t dc, uint8_t busy, uint8_t reset) : _cs(cs),
                                                                            _dc(dc),
                                                                            _busy(busy),
                                                                            _reset(reset)
{
}

/******************************************************************************
function :  Turn On Display
parameter:
******************************************************************************/
void EPD_7in3e::turnOnDisplay(void)
{

    sendCommand(0x04); // POWER_ON
    readBusyH();

    // Second setting
    sendCommand(0x06);
    sendData(0x6F);
    sendData(0x1F);
    sendData(0x17);
    sendData(0x49);

    sendCommand(0x12); // DISPLAY_REFRESH
    sendData(0x00);
    readBusyH();

    sendCommand(0x02); // POWER_OFF
    sendData(0X00);
    readBusyH();
}

/******************************************************************************
function :  Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_7in3e::init(void)
{
    pinMode(_busy, INPUT);
    pinMode(_reset, OUTPUT);
    pinMode(_dc, OUTPUT);
    pinMode(_cs, OUTPUT);

    digitalWrite(_cs, HIGH);

    reset();
    readBusyH();
    delay(30);

    sendCommand(0xAA); // CMDH
    sendData(0x49);
    sendData(0x55);
    sendData(0x20);
    sendData(0x08);
    sendData(0x09);
    sendData(0x18);

    sendCommand(0x01); //
    sendData(0x3F);

    sendCommand(0x00);
    sendData(0x5F);
    sendData(0x69);

    sendCommand(0x03);
    sendData(0x00);
    sendData(0x54);
    sendData(0x00);
    sendData(0x44);

    sendCommand(0x05);
    sendData(0x40);
    sendData(0x1F);
    sendData(0x1F);
    sendData(0x2C);

    sendCommand(0x06);
    sendData(0x6F);
    sendData(0x1F);
    sendData(0x17);
    sendData(0x49);

    sendCommand(0x08);
    sendData(0x6F);
    sendData(0x1F);
    sendData(0x1F);
    sendData(0x22);

    sendCommand(0x30);
    sendData(0x03);

    sendCommand(0x50);
    sendData(0x3F);

    sendCommand(0x60);
    sendData(0x02);
    sendData(0x00);

    sendCommand(0x61);
    sendData(0x03);
    sendData(0x20);
    sendData(0x01);
    sendData(0xE0);

    sendCommand(0x84);
    sendData(0x01);

    sendCommand(0xE3);
    sendData(0x2F);

    sendCommand(0x04); // PWR on
    readBusyH();       // waiting for the electronic paper IC to release the idle signal
}

/******************************************************************************
function :  Clear screen
parameter:
******************************************************************************/
void EPD_7in3e::clear(int width, int height, uint8_t color)
{
    uint16_t Width, Height;
    Width = (width % 2 == 0) ? (width / 2) : (width / 2 + 1);
    Height = height;

    sendCommand(0x10);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            sendData((color << 4) | color);
        }
    }

    turnOnDisplay();
}

void EPD_7in3e::draw(int width, int height, std::function<uint8_t(int, int)> lambda)
{
    uint8_t buffer[width / 2];

    sendCommand(0x10);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += 2)
        {
            auto p1 = lambda(x + 0, y);
            auto p2 = lambda(x + 1, y);
            buffer[x / 2] = (p1 << 4) | (p2 << 0);
        }
        sendData(buffer, width / 2);
    }

    turnOnDisplay();
}

/******************************************************************************
function :  Enter sleep mode
parameter:
******************************************************************************/
void EPD_7in3e::sleep(void)
{
    sendCommand(0X02); // DEEP_SLEEP
    sendData(0x00);
    readBusyH();

    sendCommand(0x07); // DEEP_SLEEP
    sendData(0XA5);
}
