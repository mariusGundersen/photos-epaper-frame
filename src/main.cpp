#include <Arduino.h>
#include <EPD_7in3e.h>
#include <Adafruit_ST7789.h>
#include <WiFiManager.h>
#include <qrcode.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Epaper.h>
#include <GithubUpdate.h>

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */

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
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

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
  wm.resetSettings();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
  {
    // try to connect, if fails, show message screen and go back to sleep
    wm.setConnectTimeout(0);
    wm.setEnableConfigPortal(false);
  }
  else
  {
    // try to connect, if fails, show config portal for 2 minutes, then show message screen and go back to sleep
    wm.setConfigPortalTimeout(120);
    wm.setEnableConfigPortal(true);
    wm.setCaptivePortalEnable(true);
    wm.setAPCallback([&](WiFiManager *wm)
                     { wifiScreen(gfx, "bilderamme", "password"); });
    // TODO: show different qrcode when a client has connected

    wm.setSaveConfigCallback([&]()
                             {      
      gfx.fillScreen(EPD_7IN3E_WHITE);
      gfx.setFont(&FreeSans24pt7b);
      gfx.setTextColor(EPD_7IN3E_BLACK);
      gfx.printCentredText("Koblet til WiFi", gfx.width()/2, gfx.height()/2);
      gfx.updateDisplay(); });
  }

  if (!wm.autoConnect("bilderamme", "password"))
  {
    gfx.fillScreen(EPD_7IN3E_WHITE);
    gfx.setFont(&FreeSans24pt7b);
    gfx.setTextColor(EPD_7IN3E_BLACK);
    gfx.printCentredText("Trykk på knappen for å koble til wifi", gfx.width() / 2, gfx.height() / 2);
    gfx.updateDisplay();

    // TODO: set up trigger on GPIO0

    // now sleep...
    esp_deep_sleep_start();
  }

  tft.println("Wifi connected");

  setClock();

  GithubUpdate *updater = new GithubUpdate();

  updater->update("mariusGundersen", "photos-epaper-frame", "firmware.bin");

  // log_d("e-Paper Init and Clear...\r\n");
  delay(1000);

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

  int secondsUntilNextHour = (59 - timeinfo.tm_min) * 60 + (60 - timeinfo.tm_sec);

  esp_sleep_enable_timer_wakeup(secondsUntilNextHour * uS_TO_S_FACTOR);
  log_d("Setup ESP32 to sleep for %d Seconds\n", secondsUntilNextHour);

  // esp_sleep_enable_ext1_wakeup(1 << D2, ESP_EXT1_WAKEUP_ALL_LOW);

  log_d("Going to sleep now\n");
  if (Serial)
    Serial.flush();

  esp_deep_sleep_start();
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
