#include <Arduino.h>
#include <EPD_7in3e.h>
#include <Adafruit_ST7789.h>
#include <WiFiManager.h>
#include <qrcode.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Epaper.h>
#include <GithubUpdate.h>
#include <JPEGDEC.h>

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */

Preferences prefs;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, -1);
uint8_t x = 0;
uint16_t rgb = 0;

const char *cloudflareRootCACert = "-----BEGIN CERTIFICATE-----\n"
                                   "MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD\n"
                                   "VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG\n"
                                   "A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw\n"
                                   "WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz\n"
                                   "IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi\n"
                                   "AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi\n"
                                   "QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR\n"
                                   "HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW\n"
                                   "BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D\n"
                                   "9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8\n"
                                   "p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD\n"
                                   "-----END CERTIFICATE-----";

void doOTA()
{
  GithubUpdate updater;

  updater.update("mariusGundersen", "photos-epaper-frame", "firmware.bin");
}

void wifiScreen(Epaper *gfx, const char *ssid, const char *password)
{
  String text = String("WIFI:T:WPA:S:");
  text.concat(ssid);
  text.concat(";P:");
  text.concat(password);
  text.concat(";;");
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(4)];

  qrcode_initText(&qrcode, qrcodeData, 4, ECC_MEDIUM, text.c_str());

  gfx->fillScreen(EPD_7IN3E_WHITE);
  gfx->setFont(&FreeSans24pt7b);
  gfx->setTextColor(EPD_7IN3E_BLACK);

  uint8_t scale = 4;
  uint16_t qrcodeSize = qrcode.size * scale;
  uint16_t left = (gfx->width() - qrcodeSize) / 2;
  uint16_t top = (gfx->height() - qrcodeSize) / 2;
  gfx->printCentredText("Koble til WiFi", gfx->width() / 2, top / 2);

  for (uint8_t y = 0; y < qrcode.size; y++)
  {
    for (uint8_t x = 0; x < qrcode.size; x++)
    {
      gfx->fillRect(
          left + x * scale,
          top + y * scale,
          scale,
          scale,
          qrcode_getModule(&qrcode, x, y) ? EPD_7IN3E_BLACK : EPD_7IN3E_WHITE);
    }
  }

  gfx->setFont(&FreeSans12pt7b);
  char buffer[40];
  sprintf(buffer, "SSID: %s", ssid);
  uint16_t h = gfx->printCentredText(buffer, gfx->width() / 2, top + qrcodeSize + 10, false);
  h += 3; // some padding between the lines
  sprintf(buffer, "Passord: %s", password);
  gfx->printCentredText(buffer, gfx->width() / 2, top + qrcodeSize + h + 10, false);

  gfx->updateDisplay();
}

void setClock()
{
  // TODO: replace with ezTime
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();

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

JPEGDEC jpeg;
Epaper *gfx;
int drawImg(JPEGDRAW *pDraw)
{
  gfx->drawRGBBitmap(
      pDraw->x,
      pDraw->y,
      pDraw->pPixels,
      pDraw->iWidth,
      pDraw->iHeight);
  return 1;
}

bool getJpeg(String url)
{
  NetworkClientSecure client;

  client.setCACert(cloudflareRootCACert);

  HTTPClient http;
  http.addHeader("Authorization", prefs.getString("token"));
  http.addHeader("If-None-Match", prefs.getString("ETag"));

  const char *headerKeys[] = {"ETag"};
  const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
  http.collectHeaders(headerKeys, headerKeysCount);

  http.useHTTP10();
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_NOT_MODIFIED)
  {
    log_d("Not modified");
    return false;
  }
  else if (httpCode != HTTP_CODE_OK)
  {
    log_d("Failed to fetch the JPEG image, HTTP code: %d\n", httpCode);

    return false;
  }

  int size = http.getSize();
  log_d("Size: %d\n", size);

  if (size == 0)
  {
    log_d("Got zero bytes");
    return false;
  }

  String etag = http.header("ETag");
  NetworkClient *stream = http.getStreamPtr();
  uint8_t *buffer = new uint8_t[size];
  size_t bytesRead = 0;
  while (bytesRead < size)
  {
    bytesRead += stream->readBytes(buffer + bytesRead, size);
  }
  log_d("downloaded image\n");
  http.end();

  if (jpeg.openRAM(buffer, size, drawImg))
  {
    log_d("opened jpeg\n");
    log_d("Image size: %d x %d, orientation: %d, bpp: %d\n", jpeg.getWidth(), jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());

    unsigned long lTime = micros();
    if (jpeg.decode(0, 0, 0))
    {
      lTime = micros() - lTime;
      log_d("Decoded image in %d us\n", (int)lTime);

      prefs.putString("ETag", etag);
      jpeg.close();
      delete[] buffer;
      return true;
    }
    else
    {
      log_d("Failed to decode image %d", jpeg.getLastError());
      jpeg.close();
      delete[] buffer;
      return false;
    }
  }
  else
  {
    log_d("Could not open jpeg %d\n", jpeg.getLastError());

    return false;
  }
}

void setup()
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  Serial.begin();

  delay(1000);

  /////////TFT////////////

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

  ///////////// PREFS ////////////////
  prefs.begin("6-color-epd");

  // SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  gfx = new Epaper(A5, A4, A3, A2, 800, 480);
  gfx->setRotation(3);

  gfx->fillScreen(EPD_7IN3E_BLACK);
  gfx->setTextColor(EPD_7IN3E_RED);
  gfx->setFont(&FreeSans24pt7b);
  gfx->printCentredText("TESTING");
  gfx->updateDisplay();

  tft.println("Go!");

  ////////////////////////////////////////

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  WiFiManagerParameter token("token", "Token", prefs.getString("token").c_str(), 40);
  wm.addParameter(&token);

  // wm.resetSettings();

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
                              prefs.putString("token", token.getValue());
      gfx->fillScreen(EPD_7IN3E_WHITE);
      gfx->setFont(&FreeSans24pt7b);
      gfx->setTextColor(EPD_7IN3E_BLACK);
      gfx->printCentredText("Koblet til WiFi");
      gfx->updateDisplay(); });
  }

  if (!wm.autoConnect("bilderamme", "password"))
  {
    gfx->fillScreen(EPD_7IN3E_WHITE);
    gfx->setFont(&FreeSans24pt7b);
    gfx->setTextColor(EPD_7IN3E_BLACK);
    gfx->printCentredText("Trykk på knappen for å koble til wifi");
    gfx->updateDisplay();

    // TODO: set up trigger on GPIO0

    // now sleep...
    esp_deep_sleep_start();
  }

  tft.println("Wifi connected");

  setClock();

  doOTA();

  // log_d("e-Paper Init and Clear...\r\n");
  delay(1000);

  tft.println("Drawing in black");
  delay(1000);

  if (getJpeg("https://6-color-epd.pages.dev/photo"))
  {
    gfx->dither();
    gfx->updateDisplay();
  }

  delay(5000);

  // gfx->fillScreen(EPD_7IN3E_WHITE);
  gfx->setFont(&FreeSans24pt7b);
  gfx->setTextColor(EPD_7IN3E_YELLOW);

  time_t nowSecs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);

  char *timeStr = asctime(&timeinfo);
  Serial.println(timeStr);

  gfx->printCentredText(timeStr);
  // gfx->updateDisplay();

  int minutesUntilNextHour = 59 - timeinfo.tm_min;
  if (minutesUntilNextHour < 5)
  {
    minutesUntilNextHour += 60;
  }
  int secondsUntilNextHour = minutesUntilNextHour * 60 + (60 - timeinfo.tm_sec);

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
