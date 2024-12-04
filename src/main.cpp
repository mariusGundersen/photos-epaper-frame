#include <Arduino.h>
#include <EPD_7in3e.h>
#include <Adafruit_ST7789.h>
#include <WiFiManager.h>
#include <qrcode.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Epaper.h>
#include <GithubUpdate.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, -1);
uint8_t x = 0;
uint16_t rgb = 0;

void wifiScreen(Epaper &gfx, const char *ssid, const char *password)
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
  gfx.setFont(&FreeSans24pt7b);
  gfx.setTextColor(EPD_7IN3E_BLACK);

  uint8_t scale = 4;
  uint16_t qrcodeSize = qrcode.size * scale;
  uint16_t left = (gfx.width() - qrcodeSize) / 2;
  uint16_t top = (gfx.height() - qrcodeSize) / 2;
  gfx.printCentredText("Koble til WiFi", gfx.width() / 2, top / 2);

  for (uint8_t y = 0; y < qrcode.size; y++)
  {
    for (uint8_t x = 0; x < qrcode.size; x++)
    {
      gfx.fillRect(
          left + x * scale,
          top + y * scale,
          scale,
          scale,
          qrcode_getModule(&qrcode, x, y) ? EPD_7IN3E_BLACK : EPD_7IN3E_WHITE);
    }
  }

  gfx.setFont(&FreeSans12pt7b);
  char buffer[40];
  sprintf(buffer, "SSID: %s", ssid);
  uint16_t h = gfx.printCentredText(buffer, gfx.width() / 2, top + qrcodeSize + 10, false);
  h += 3; // some padding between the lines
  sprintf(buffer, "Passord: %s", password);
  gfx.printCentredText(buffer, gfx.width() / 2, top + qrcodeSize + h + 10, false);

  gfx.updateDisplay();
}

void setClock()
{
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
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

  tft.println("Set...");

  // SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  Epaper gfx = Epaper(A5, A4, A3, A2);
  gfx.setRotation(2); // rotate upside down

  tft.println("Go!");

  ////////////////////////////////////////

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  // wm.resetSettings();
  wm.setConnectTimeout(0);
  wm.setEnableConfigPortal(false);
  if (!wm.autoConnect("bilderamme", "password"))
  {
    pinMode(GPIO_NUM_0, INPUT_PULLUP);
    gfx.fillScreen(EPD_7IN3E_WHITE);
    gfx.setFont(&FreeSans24pt7b);
    gfx.setTextColor(EPD_7IN3E_BLACK);
    gfx.printCentredText("Trykk på knappen for å koble til wifi", gfx.width() / 2, gfx.height() / 2);
    gfx.updateDisplay();

    while (digitalRead(GPIO_NUM_0) == HIGH)
    {
      delay(100);
    }

    wm.setConfigPortalTimeout(120);
    wm.setEnableConfigPortal(true);
    wm.setCaptivePortalEnable(true);
    wm.setAPCallback([&](WiFiManager *wm)
                     { wifiScreen(gfx, "bilderamme", "password"); });
    wm.autoConnect("bilderamme", "password");
  }

  tft.println("Wifi connected");

  setClock();

  GithubUpdate *updater = new GithubUpdate();

  updater->update("mariusGundersen", "photos-epaper-frame", "firmware.bin");

  // log_d("e-Paper Init and Clear...\r\n");
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

  gfx.dither();

  gfx.updateDisplay();

  delay(5000);

  // gfx.fillScreen(EPD_7IN3E_WHITE);
  gfx.setFont(&FreeSans24pt7b);
  gfx.setTextColor(EPD_7IN3E_BLACK);

  time_t nowSecs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);

  char *timeStr = asctime(&timeinfo);
  Serial.println(timeStr);

  gfx.printCentredText(timeStr, gfx.width() / 2, gfx.height() / 2);
  gfx.updateDisplay();
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
