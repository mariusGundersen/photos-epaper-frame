#include <Arduino.h>
#include <EPD_7in3e.h>
#include <WiFiManager.h>
#include <qrcode.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Epaper.h>
#include <GithubUpdate.h>
#include <JPEGDEC.h>
#include "Adafruit_MAX1704X.h"

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */

Preferences prefs;
Epaper *gfx;
Adafruit_MAX17048 maxlipo;
uint8_t x = 0;
uint16_t rgb = 0;

RTC_DATA_ATTR unsigned int counter = 0;

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

void goHereScreen()
{
  String hostname = WiFi.AP.gatewayIP().toString();
  String text = String("http://");
  text.concat(hostname);
  text.concat("/wifi");
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
  gfx->printCentredText("Go to this page", gfx->width() / 2, top / 2);

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
  uint16_t h = gfx->printCentredText(text.c_str(), gfx->width() / 2, top + qrcodeSize + 10, false);

  gfx->updateDisplay();
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
  gfx->printCentredText("Connect to WiFi", gfx->width() / 2, top / 2);

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
  sprintf(buffer, "Password: %s", password);
  gfx->printCentredText(buffer, gfx->width() / 2, top + qrcodeSize + h + 10, false);

  gfx->updateDisplay();
}

void sleepUntilNextHour(bool untilTomorrrow = true)
{
  time_t nowSecs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);

  int hoursUntilTomorrowMorning = untilTomorrrow ? 6 - timeinfo.tm_hour : 0;
  if (hoursUntilTomorrowMorning < 0)
  {
    hoursUntilTomorrowMorning += 24;
  }

  int minutesUntilNextHour = 59 - timeinfo.tm_min;
  if (minutesUntilNextHour < 5)
  {
    minutesUntilNextHour += 60;
  }

  int secondsUntilNextMinute = 60 - timeinfo.tm_sec;
  int secondsToSleep = (hoursUntilTomorrowMorning * 60 + minutesUntilNextHour) * 60 + secondsUntilNextMinute;

  esp_sleep_enable_timer_wakeup(secondsToSleep * uS_TO_S_FACTOR);
  log_d("Setup ESP32 to sleep for %d Seconds\n", secondsToSleep);

  esp_sleep_enable_ext1_wakeup(1 << GPIO_NUM_0, ESP_EXT1_WAKEUP_ANY_HIGH);

  log_d("Going to sleep now\n");
  if (Serial)
    Serial.flush();

  esp_deep_sleep_start();
}

void connectToWifi(esp_sleep_wakeup_cause_t wakeup_reason, bool reset = false)
{
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  WiFiManagerParameter token("token", "Token", prefs.getString("token").c_str(), 40);
  wm.addParameter(&token);

  if (reset)
  {
    wm.resetSettings();
  }

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

    network_event_handle_t event = WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info)
                                                {
                  WiFi.removeEvent(event);
                  goHereScreen(); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);

    wm.setSaveConfigCallback([&]()
                             {
                              prefs.putString("token", token.getValue());
      gfx->fillScreen(EPD_7IN3E_WHITE);
      gfx->setFont(&FreeSans24pt7b);
      gfx->setTextColor(EPD_7IN3E_BLACK);
      gfx->printCentredText("Connect to WiFi");
      gfx->updateDisplay(); });
  }

  if (wm.autoConnect("bilderamme", "password"))
  {
    counter = 0;
  }
  else
  {
    counter++;
    if (counter > 5 || wakeup_reason != ESP_SLEEP_WAKEUP_TIMER)
    {
      gfx->fillScreen(EPD_7IN3E_WHITE);
      gfx->setFont(&FreeSans24pt7b);
      gfx->setTextColor(EPD_7IN3E_BLACK);
      gfx->printCentredText("Press Reset button");
      gfx->updateDisplay();

      // make sure it refreshes the screen when it reconnects
      prefs.remove("ETag");
    }

    // now sleep for 1 hour then retry
    sleepUntilNextHour(false);
  }
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
  if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER)
  {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  Serial.begin();

  /////////TFT////////////

  Serial.println("Started");

  ///////////// PREFS ////////////////
  prefs.begin("6-color-epd");

  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  gfx = new Epaper(A5, A4, A3, A2, 800, 480);
  gfx->setRotation(1);

  ////////////////////////////////////////

  while (!maxlipo.begin())
  {
    Serial.println(F("Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
    delay(2000);
  }
  Serial.print(F("Found MAX17048"));
  Serial.print(F(" with Chip ID: 0x"));
  Serial.println(maxlipo.getChipID(), HEX);

  return;

  connectToWifi(wakeup_reason);
  setClock();
  doOTA();

  if (getJpeg("https://6-color-epd.pages.dev/photo"))
  {
    gfx->dither();
    gfx->updateDisplay();
  }

  sleepUntilNextHour();
}

void loop()
{
  float cellVoltage = maxlipo.cellVoltage();
  if (isnan(cellVoltage))
  {
    Serial.println("Failed to read cell voltage, check battery is connected!");
    delay(2000);
    return;
  }
  Serial.print(F("Batt Voltage: "));
  Serial.print(cellVoltage, 3);
  Serial.println(" V");
  Serial.print(F("Batt Percent: "));
  Serial.print(maxlipo.cellPercent(), 1);
  Serial.println(" %");
  Serial.println();

  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);

  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
