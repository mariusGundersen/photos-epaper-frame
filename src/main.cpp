#include <Arduino.h>
#include <EPD_7in3e.h>
#include <Adafruit_ST7789.h>
#include <Dither.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, -1);
uint8_t x = 0;
uint16_t rgb = 0;
GFXcanvas16 epd = GFXcanvas16(EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT);

RGB palette[6] = {
    ST77XX_BLACK,  // black
    ST77XX_WHITE,  // white
    ST77XX_YELLOW, // yellow
    ST77XX_RED,
    ST77XX_BLUE,
    ST77XX_GREEN,
};

FloydSteinberg floydSteinberg(6, palette);

void setup()
{
  Serial.begin();
  delay(1000);
  Serial.println("Started");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  tft.init(135, 240);

  delay(500);

  tft.enableDisplay(true);

  tft.fillRect(0, 0, 135, 240, ST77XX_BLUE);

  Serial.println(("TFT INIT"));

  tft.drawLine(70, 0, 70, 239, ST77XX_GREEN);

  Debug("EPD_7IN3E_test Demo\r\n");
  if (DEV_Module_Init() != 0)
  {
    return;
  }

  Debug("e-Paper Init and Clear...\r\n");
  EPD_7IN3E_Init();
  EPD_7IN3E_Clear(EPD_7IN3E_WHITE); // WHITE
  DEV_Delay_ms(1000);

  epd.fillScreen(EPD_7IN3E_BLACK);
  epd.fillRect(0, 0, 133, 200, EPD_7IN3E_BLACK); // this line causes it to crash. why?

  // epd.fillRect(0, 133, 133, EPD_7IN3E_HEIGHT, EPD_7IN3E_BLUE);
  /*
  epd.drawRect(0, 266, 134, EPD_7IN3E_HEIGHT, EPD_7IN3E_RED);
  epd.drawRect(0, 400, 133, EPD_7IN3E_HEIGHT, EPD_7IN3E_GREEN);
  epd.drawRect(0, 533, 133, EPD_7IN3E_HEIGHT, EPD_7IN3E_YELLOW);
  epd.drawRect(0, 666, 134, EPD_7IN3E_HEIGHT, EPD_7IN3E_WHITE);

  draw([&](int x, int y)
       { return epd.getPixel(x, y); });

  // EPD_7IN3E_Show7Block();
  */

  EPD_7IN3E_Sleep();

  SPI.endTransaction();

  tft.enableDisplay(true);
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);

  tft.drawLine(x, 0, x, 239, rgb++);

  x = (x + 1) % 135;

  Serial.println("Blink");

  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
