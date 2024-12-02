#include <Arduino.h>
#include <EPD_7in3e.h>
#include <Adafruit_ST7789.h>
#include <Dither.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, -1);
uint8_t x = 0;
uint16_t rgb = 0;
// GFXcanvas16 epd; // EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT);

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

  GFXcanvas16 gfx = GFXcanvas16(EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT);

  tft.enableDisplay(true);

  tft.fillRect(0, 0, 135, 240, ST77XX_BLUE);

  Serial.println(("TFT INIT"));

  // tft.drawLine(70, 0, 70, 239, ST77XX_GREEN);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 0);
  tft.println("Ready!");
  tft.println("Set...");
  tft.println("Go!");

  // SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  // log_d("EPD_7IN3E_test Demo\r\n");
  // GPIO_Config();
  EPD_7in3e epd = EPD_7in3e(A5, A4, A3, A2);

  tft.println("Ready");

  // log_d("e-Paper Init and Clear...\r\n");
  epd.init();
  epd.clear(EPD_7IN3E_WHITE);
  delay(1000);
  tft.println("Checking size");

  // log_d("Shown white, now drawing\n");

  // log_d("Shown 7 blocks, now drawing\r\n");
  tft.printf("Buffer is at %d", gfx.getBuffer());

  Serial.printf("Size of buffer %d", gfx.getBuffer());

  tft.println("Drawing in black");
  delay(1000);

  gfx.fillScreen(ST77XX_BLACK);
  gfx.fillRect(0, 0, 133, EPD_7IN3E_HEIGHT, ST77XX_BLACK);
  gfx.fillRect(133, 0, 133, EPD_7IN3E_HEIGHT / 2, ST77XX_BLUE);
  gfx.fillRect(133, EPD_7IN3E_HEIGHT / 2, 133, EPD_7IN3E_HEIGHT / 2, ST77XX_CYAN);
  gfx.fillRect(266, 0, 134, EPD_7IN3E_HEIGHT / 2, ST77XX_RED);
  gfx.fillRect(266, EPD_7IN3E_HEIGHT / 2, 134, EPD_7IN3E_HEIGHT / 2, ST77XX_MAGENTA);
  gfx.fillRect(400, 0, 133, EPD_7IN3E_HEIGHT, ST77XX_GREEN);
  gfx.fillRect(533, 0, 133, EPD_7IN3E_HEIGHT, ST77XX_YELLOW);
  gfx.fillRect(666, 0, 134, EPD_7IN3E_HEIGHT, ST77XX_WHITE);

  floydSteinberg.dither(EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT, gfx.getBuffer());

  epd.draw([&](int x, int y)
           { return gfx.getPixel(x, y); });

  epd.sleep();

  delay(5000);

  epd.init();

  gfx.fillScreen(RGB(0, 0xff, 0xff));

  /* for (int y = 0; y < EPD_7IN3E_HEIGHT; y++)
  {
    for (int x = 0; x < EPD_7IN3E_WIDTH; x++)
    {
      gfx.writePixel(x, y, RGB(x * 0x1f / EPD_7IN3E_WIDTH, y * 0x3f / EPD_7IN3E_HEIGHT, 0xf));
    }
  }
 */
  floydSteinberg.dither(EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT, gfx.getBuffer());

  epd.draw([&](int x, int y)
           { return gfx.getPixel(x, y); });

  epd.sleep();

  // SPI.endTransaction();

  // tft.enableDisplay(true);
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
