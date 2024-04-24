#include <Arduino.h>
#include <SPI.h>

#define EPD_SIZE(value) value >> 8, value & 0xff

class Epd
{
    int DC_Pin;
    int BUSY_Pin;
    int RES_Pin;
    SPIClass spi;

public:
    Epd(int BUSY_Pin, int RES_Pin, int DC_Pin, int CS_Pin, int SCK_Pin, int MISO_Pin, int MOSI_Pin)
    {
        pinMode(BUSY_Pin, INPUT);
        pinMode(RES_Pin, OUTPUT);
        pinMode(DC_Pin, OUTPUT);
        pinMode(CS_Pin, OUTPUT);
        pinMode(SCK_Pin, OUTPUT);
        pinMode(MOSI_Pin, OUTPUT);
        spi = SPIClass();
        spi.begin(SCK_Pin, MISO_Pin, MOSI_Pin, CS_Pin);
        this->DC_Pin = DC_Pin;
        this->BUSY_Pin = BUSY_Pin;
        this->RES_Pin = RES_Pin;
    }

    void write(unsigned char command)
    {
        digitalWrite(DC_Pin, 0); // command write
        spi.write(command);
    }

    void W21_WriteDATA(unsigned char data)
    {
        digitalWrite(DC_Pin, 1); // data write
        spi.write(data);
    }

    void write(uint8_t command, const uint8_t *data, size_t bytes)
    {
        digitalWrite(DC_Pin, 0); // command write
        spi.write(command);

        digitalWrite(DC_Pin, 1); // data write
        spi.writeBytes(data, bytes);
    }

    void write(uint8_t command, uint8_t data)
    {
        digitalWrite(DC_Pin, 0); // command write
        spi.write(command);

        digitalWrite(DC_Pin, 1); // data write
        spi.write(data);
    }

    template <size_t N>
    void write(uint8_t command, const uint8_t (&data)[N])
    {
        write(command, data, N);
    }

    void init(void)
    {
        // Electronic paper IC reset
        digitalWrite(RES_Pin, 0); // Module reset
        delay(100);               // At least 10ms
        digitalWrite(RES_Pin, 1);
        delay(100);

        // POWER SETTING
        write(0x01, (const uint8_t[]){0x37, 0x00, 0x23, 0x23});

        // PANNEL SETTING
        write(0x00, (const uint8_t[]){0xef, 0x08});

        // PFS (Power off sequence)
        write(0x03, 0x00);

        // BTST (Booster Soft Start)
        write(0x06, (const uint8_t[]){0xC7, 0xC7, 0x1D});

        // PLL setting
        write(0x30, 0x3C);

        // TSE (Temperature Sensor Enable)
        write(0X41, 0x00);

        // VCOM AND DATA INTERVAL SETTING
        write(0X50, 0x37);

        // TCON SETTING
        write(0X60, 0x22);

        // TCON SETTING
        write(0X60, 0x22);

        // TRES (Resolution settings, 600x448)
        write(0x61, (const uint8_t[]){EPD_SIZE(600), EPD_SIZE(448)});

        // PWS (Power saving?)
        write(0xE3, 0xAA);

        // PWR on
        write(0x04);
        wait_while_busy(); // waiting for the electronic paper IC to release the idle signal
    }

    void refresh(void)
    {
        write(0x12);            // DISPLAY REFRESH
        delayMicroseconds(200); //!!!The delay here is necessary, 200uS at least!!!
        wait_while_busy();
    }

    void sleep(void)
    {
        write(0X02); // power off
        wait_while_busy();
        write(0X07, 0xA5); // deep sleep
    }

    void wait_while_busy(void)
    {
        while (!digitalRead(BUSY_Pin))
            ;
    }

    /*
    Black   /// 000
    White   /// 001
    Green   /// 010
    Blue    /// 011
    Red     /// 100
    Yellow  /// 101
    Orange  /// 110
    CLear   /// 111
    */
    void draw_color(unsigned char color)
    {
        write(0x10);
        digitalWrite(DC_Pin, 1); // data write
        spi.writePattern(&color, 1, 300 * 448);

        // Refresh
        refresh();
    }

    void draw(std::function<uint8_t(int, int)> lambda)
    {
        uint8_t buffer[300];

        write(0x10);

        digitalWrite(DC_Pin, 1); // data write
        for (int y = 0; y < 448; y++)
        {
            for (int x = 0; x < 600; x += 2)
            {
                auto p1 = lambda(x + 0, y);
                auto p2 = lambda(x + 1, y);
                buffer[x / 2] = (p1 << 4) | (p2 << 0);
            }
            spi.writeBytes(buffer, 300);
        }

        refresh();
    }
};