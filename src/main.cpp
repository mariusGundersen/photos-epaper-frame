#include <Arduino.h>
#include <EPD_7in3e.h>
#include <Adafruit_ST7789.h>
#include <Dither.h>
#include <WiFiManager.h>
#include <qrcode.h>
#include <Fonts/FreeSans24pt7b.h>

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

void wifiScreen(GFXcanvas16 &gfx, const char *ssid, const char *password)
{

  String text = String("WIFI:T:WPA:S:");
  text.concat(ssid);
  text.concat(";P:");
  text.concat(password);
  text.concat(";;");
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(4)];

  qrcode_initText(&qrcode, qrcodeData, 4, ECC_MEDIUM, text.c_str());

  gfx.fillScreen(EPD_7IN3E_WHITE);
  gfx.setTextSize(1);
  gfx.setFont(&FreeSans24pt7b);
  gfx.setTextColor(EPD_7IN3E_BLACK);

  uint8_t scale = 4;
  uint16_t dx = (gfx.width() - qrcode.size * scale) / 2;
  uint16_t dy = (gfx.height() - qrcode.size * scale) / 2;

  int16_t x1, y1;
  uint16_t w, h;
  gfx.getTextBounds("Koble til wifi:", 0, 0, &x1, &y1, &w, &h);
  gfx.setCursor(gfx.width() / 2 - w / 2, dy / 2 - h / 2);
  gfx.print("Koble til wifi:");

  for (uint8_t y = 0; y < qrcode.size; y++)
  {
    for (uint8_t x = 0; x < qrcode.size; x++)
    {
      gfx.fillRect(
          dx + x * scale,
          dy + y * scale,
          scale,
          scale,
          qrcode_getModule(&qrcode, x, y) ? EPD_7IN3E_BLACK : EPD_7IN3E_WHITE);
    }
  }
}

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

  // tft.drawLine(70, 0, 70, 239, ST77XX_GREEN);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 0);
  tft.println("Ready!");

  GFXcanvas16 gfx = GFXcanvas16(EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT);

  tft.println("Set...");

  // SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  EPD_7in3e epd = EPD_7in3e(A5, A4, A3, A2);

  tft.println("Go!");

  ////////////////////////////////////////

  wifiScreen(gfx, "bilderamme", "password");

  epd.init();

  epd.draw([&](int x, int y)
           { return gfx.getPixel(x, y); });

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConnectTimeout(0);
  // wm.setConfigPortalTimeout(60);
  wm.autoConnect("bilderamme", "password");

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
