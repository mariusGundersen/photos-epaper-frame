#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <JPEGDEC.h>
#include <SPI.h>
#include "dither.h"
#include "Preferences.h"

// IO settings
int BUSY_Pin = D5;
int RES_Pin = D0;
int DC_Pin = D3;
int CS_Pin = D1;
int SCK_Pin = D8;
int SDO_Pin = D9;
int SDI_Pin = D10;

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5 * 60      /* Time ESP32 will go to sleep (in seconds) */

#define EPD_SIZE(value) value & 0xff, value >> 8

#define isEPD_W21_BUSY digitalRead(BUSY_Pin)

// 8bit
#define Black 0x00  /// 000
#define White 0x11  /// 001
#define Green 0x22  /// 010
#define Blue 0x33   /// 011
#define Red 0x44    /// 100
#define Yellow 0x55 /// 101
#define Orange 0x66 /// 110
#define Clean 0x77  /// 111

// 4bit
#define black 0x00  /// 000
#define white 0x01  /// 001
#define green 0x02  /// 010
#define blue 0x03   /// 011
#define red 0x04    /// 100
#define yellow 0x05 /// 101
#define orange 0x06 /// 110
#define clean 0x07  /// 111

////////FUNCTION//////
void EPD_W21_WriteDATA(unsigned char command);
void EPD_W21_WriteCMD(unsigned char command);
// EPD
void EPD_W21_Init(void);
void EPD_init(void);
void PIC_display(const unsigned char *picData);
void draw4bit(uint8_t *picData);
void EPD_sleep(void);
void EPD_refresh(void);
void lcd_chkstatus(void);
void PIC_display_Clear(void);
void EPD_horizontal(void);
void EPD_vertical(void);
void Acep_color(unsigned char color);

Preferences prefs;
WiFiManager wm;
SPIClass epd_spi;
JPEGDEC jpeg;
unsigned char *epd_buffer;
uint16_t *pixels;

uint16_t palette[7] = {
    0x0000, // rgb565(0b00000, 0b000000, 0b00000), // black
    0xffff, // rgb565(0b11111, 0b111111, 0b11111), // white
    0x3d8e, // rgb565(0b00000, 0x111111, 0x00000), // green
    0x5a56, // rgb565(0b00000, 0x000000, 0x11111), // blue
    0xc268, // rgb565(0b11111, 0x000000, 0x00000), // red
    0xff87, // rgb565(0b11111, 0x111111, 0x00000), // yellow
    0xf569, // rgb565(0b11111, 0x011111, 0x00000)  // orange
};

FloydSteinberg floydSteinberg(7, palette);

int drawImg(JPEGDRAW *pDraw)
{
  int xOffset = pDraw->x;
  int yOffset = pDraw->y;
  int w = pDraw->iWidth;
  int h = pDraw->iHeight;

  for (int y = 0; y < h; y++)
  {
    for (int x = 0; x < w; x++)
    {
      pixels[(yOffset + y) * 600 + (xOffset + x)] = pDraw->pPixels[y * w + x];
    }
  }
  return 1;
}

void setup()
{
  pinMode(BUSY_Pin, INPUT);
  pinMode(RES_Pin, OUTPUT);
  pinMode(DC_Pin, OUTPUT);
  pinMode(CS_Pin, OUTPUT);
  pinMode(SCK_Pin, OUTPUT);
  pinMode(SDI_Pin, OUTPUT);

  Serial.begin(115200);

  delay(5000);

  wm.autoConnect("e-ink", "password");

  Serial.println("WiFi - Connected");

  // WiFi.mode(WIFI_MODE_APSTA);
  // WiFi.softAP("e-ink", "password");

  prefs.begin("e-ink");

  // prefs.putString("imageUrl", "...");

  HTTPClient http;
  http.begin(prefs.getString("imageUrl"));
  int httpCode = http.GET();

  if (httpCode <= 0)
  {
    Serial.print("Failed to fetch the JPEG image, HTTP code: ");
    Serial.println(httpCode);
    return;
  }

  epd_spi = SPIClass();
  epd_spi.begin(SCK, MISO, MOSI, CS_Pin);

  int size = http.getSize();
  Serial.printf("Size: %d\n", size);
  WiFiClient *stream = http.getStreamPtr();
  uint8_t *buffer = (uint8_t *)malloc(size * sizeof(uint8_t));
  pixels = (uint16_t *)malloc(600 * 448 * sizeof(uint16_t));
  uint8_t *write = buffer;
  Serial.printf("streaming %d\n", stream->available());
  while (http.connected() && stream->available())
  {
    write += stream->readBytes(write, size - (write - buffer));
    Serial.printf("read %d\n", write - buffer);
  }
  Serial.println("downloaded image");

  if (jpeg.openRAM(buffer, size, drawImg))
  {
    Serial.println("opened jpeg");
    Serial.printf("Image size: %d x %d, orientation: %d, bpp: %d\n", jpeg.getWidth(),
                  jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());

    unsigned long lTime = micros();
    if (jpeg.decode(0, 0, 0))
    {
      lTime = micros() - lTime;
      Serial.printf("Decoded image in %d us\n", (int)lTime);
    }
    else
    {
      Serial.printf("Failed to decode imageg %d", jpeg.getLastError());
    }
    jpeg.close();
    free(buffer);

    epd_buffer = (unsigned char *)malloc(300 * 448 * sizeof(char));
    lTime = micros();
    floydSteinberg.dither(600, 448, pixels);
    lTime = micros() - lTime;
    Serial.printf("Dithered image in %d us\n", (int)lTime);

    for (int y = 0; y < 448; y++)
    {
      for (int x = 0; x < 600; x += 2)
      {
        int p1 = pixels[y * 600 + x + 0];
        int p2 = pixels[y * 600 + x + 1];
        epd_buffer[y * 300 + x / 2] = (p1 << 4) | (p2 << 0);
      }
    }

    EPD_init();           // EPD init
    draw4bit(epd_buffer); // EPD_picture1
    EPD_sleep();          // EPD_sleep,Sleep instruction is necessary, please do not delete!!!
  }
  else
  {
    Serial.printf("Failed to read jpeg %d", jpeg.getLastError());
  }

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");

  Serial.println("Going to sleep now");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop()
{
  /*while (1)
  {
    EPD_init();              // EPD init
    PIC_display(epd_buffer); // EPD_picture1
    EPD_sleep();             // EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(15000);            // 5s
    // EPD_horizontal
    EPD_init();       // EPD init
    EPD_horizontal(); // EPD  horizontal 7 color
    EPD_sleep();      // EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(5000);      // 5s

    EPD_init();     // EPD init
    EPD_vertical(); // EPD  vertical 7 color
    EPD_sleep();    // EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(5000);    // 5s
    while (1)
      ;

    // Clear
    EPD_init();          // EPD init
    PIC_display_Clear(); // EPD Clear
    EPD_sleep();         // EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(5000);         // 5s
    while (1)
      ;
  }*/
}

///////////////////EXTERNAL FUNCTION////////////////////////////////////////////////////////////////////////
//////////////////////SPI///////////////////////////////////

void EPD_W21_WriteCMD(unsigned char command)
{
  digitalWrite(DC_Pin, 0); // command write
  epd_spi.write(command);
}
void EPD_W21_WriteDATA(unsigned char data)
{
  digitalWrite(DC_Pin, 1); // data write
  epd_spi.write(data);
}
void EPD_write(uint8_t command, const uint8_t *data, size_t bytes)
{
  digitalWrite(DC_Pin, 0); // command write
  epd_spi.write(command);

  digitalWrite(DC_Pin, 1); // data write
  epd_spi.writeBytes(data, bytes);
}
void EPD_write(uint8_t command, uint8_t data)
{
  digitalWrite(DC_Pin, 0); // command write
  epd_spi.write(command);

  digitalWrite(DC_Pin, 1); // data write
  epd_spi.write(data);
}

template <size_t N>
void EPD_write(uint8_t command, const uint8_t (&data)[N])
{
  EPD_write(command, data, N);
}

/////////////////EPD settings Functions/////////////////////
void EPD_W21_Init(void)
{
  digitalWrite(RES_Pin, 0); // Module reset
  delay(100);               // At least 10ms
  digitalWrite(RES_Pin, 1);
  delay(100);
}
void EPD_init(void)
{
  EPD_W21_Init(); // Electronic paper IC reset

  // POWER SETTING
  EPD_write(0x01, (const uint8_t[]){0x37, 0x00, 0x23, 0x23});

  // PANNEL SETTING
  EPD_write(0x00, (const uint8_t[]){0xef, 0x08});

  // PFS
  EPD_write(0x04, 0x00);

  // boost
  EPD_write(0x06, (const uint8_t[]){0xC7, 0xC7, 0x1D});

  // PLL setting
  EPD_write(0x30, 0x3C);

  // TSE
  EPD_write(0X41, 0x00);

  // VCOM AND DATA INTERVAL SETTING
  EPD_write(0X50, 0x37);

  // TCON SETTING
  EPD_write(0X60, 0x22);

  // TCON SETTING
  EPD_write(0X60, 0x22);

  // tres
  EPD_write(0x61, (const uint8_t[]){EPD_SIZE(600), EPD_SIZE(448)});

  // PWS
  EPD_write(0xE3, 0xAA);

  // PWR on
  EPD_W21_WriteCMD(0x04);
  lcd_chkstatus(); // waiting for the electronic paper IC to release the idle signal
}
void EPD_refresh(void)
{
  EPD_W21_WriteCMD(0x12); // DISPLAY REFRESH
  delayMicroseconds(200); //!!!The delay here is necessary, 200uS at least!!!
  lcd_chkstatus();
}
void EPD_sleep(void)
{
  EPD_W21_WriteCMD(0X02); // power off
  lcd_chkstatus();
  EPD_write(0X07, 0xA5); // deep sleep
}

/**********************************display picture**********************************/
void Acep_color(unsigned char color)
{
  EPD_W21_WriteCMD(0x10);
  digitalWrite(DC_Pin, 1); // data write
  epd_spi.writePattern(&color, 1, 300 * 448);

  // Refresh
  EPD_refresh();
}
/*
White   /// 001--0XFF
Yellow  /// 101--0XFC
Orange  /// 110--0XEC
Red     /// 100--0XE0
Green   /// 010--0X35
Blue    /// 011--0X2B
Black   /// 000--0X00

//0XFF,0XFC,0XEC,0XE0,0X35,0X2B,0X00
*/
unsigned char Color_get(unsigned char color)
{
  unsigned datas;
  switch (color)
  {
  case 0xFF:
    datas = white;
    break;
  case 0xFC:
    datas = yellow;
    break;
  case 0xEC:
    datas = orange;
    break;
  case 0xE0:
    datas = red;
    break;
  case 0x35:
    datas = green;
    break;
  case 0x2B:
    datas = blue;
    break;
  case 0x00:
    datas = black;
    break;
  default:
    break;
  }
  return datas;
}
void PIC_display(const unsigned char *picData)
{
  unsigned int i, j, k;
  unsigned char temp1, temp2;
  unsigned char data_H, data_L, data;

  Acep_color(Clean); // Each refresh must be cleaned first
  EPD_W21_WriteCMD(0x10);
  for (i = 0; i < 448; i++)
  {
    k = 0;
    for (j = 0; j < 300; j++)
    {

      temp1 = picData[i * 600 + k++];
      temp2 = picData[i * 600 + k++];
      data_H = Color_get(temp1) << 4;
      data_L = Color_get(temp2);
      data = data_H | data_L;
      EPD_W21_WriteDATA(data);
    }
  }

  // Refresh
  EPD_refresh();
}

void draw4bit(uint8_t *picData)
{
  Acep_color(Clean); // Each refresh must be cleaned first

  EPD_write(0x10, picData, 448 * 300);

  // Refresh
  EPD_refresh();
}

void EPD_horizontal(void)
{
  unsigned int i, j;
  unsigned char index = 0x00;
  unsigned char const Color[8] = {Black, White, Green, Blue, Red, Yellow, Orange, Clean};

  Acep_color(Clean);      // Each refresh must be cleaned first
  EPD_W21_WriteCMD(0x10); // start to transport picture
  for (i = 0; i < 448; i++)
  {
    if ((i % 56 == 0) && (i != 0))
      index++;
    for (j = 0; j < 600 / 2; j++)
    {
      EPD_W21_WriteDATA(Color[index]);
    }
  }
  // Refresh
  EPD_refresh();
}
void EPD_vertical(void)
{
  unsigned int i, j, k;
  unsigned char const Color[8] = {Black, White, Green, Blue, Red, Yellow, Orange, Clean};

  Acep_color(Clean);      // Each refresh must be cleaned first
  EPD_W21_WriteCMD(0x10); // start to transport pictu
  for (i = 0; i < 448; i++)
  {
    for (k = 0; k < 7; k++) // 7 color
    {
      for (j = 0; j < 38; j++)
      {
        EPD_W21_WriteDATA(Color[k]);
      }
    }
    for (j = 0; j < 34; j++)
    {
      EPD_W21_WriteDATA(0x77);
    }
  }
  // Refresh
  EPD_refresh();
}

void PIC_display_Clear(void)
{
  unsigned int i, j;
  Acep_color(Clean); // Each refresh must be cleaned first
  EPD_W21_WriteCMD(0x10);
  for (i = 0; i < 448; i++)
  {
    for (j = 0; j < 300; j++)
    {
      EPD_W21_WriteDATA(White);
    }
  }

  // Refresh
  EPD_refresh();
}

void lcd_chkstatus(void)
{
  while (!isEPD_W21_BUSY)
    ;
}
