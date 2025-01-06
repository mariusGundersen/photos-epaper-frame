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

enum struct SleepDuration
{
  untilTomorrow,
  untilNextHour,
  fiveMinutes
};

struct Battery
{
  float cellPercent = 0;
  float cellVoltage = -1;
  float chargeRate = 0;
  bool isCharging()
  {
    return chargeRate > 0.1f && cellVoltage < 4.1f;
  }
  bool needsCharging()
  {
    return cellVoltage < 3.5f && chargeRate < 0.1f;
  }
};

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
  gfx->printCentredText("Go to this page", top / 2);

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
  gfx->printCentredText(text.c_str(), top + qrcodeSize + 10, false);

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
  gfx->printCentredText("Connect to WiFi", top / 2);

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
  uint16_t y = gfx->printCentredText(buffer, top + qrcodeSize + 10, false);
  y += 3; // some padding between the lines
  sprintf(buffer, "Password: %s", password);
  gfx->printCentredText(buffer, y, false);

  gfx->updateDisplay();
}

void enterDeepSleep(SleepDuration sleepDuration)
{
  time_t nowSecs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);

  int hoursToSleep = sleepDuration == SleepDuration::untilTomorrow ? 3 - timeinfo.tm_hour : 0;
  if (sleepDuration == SleepDuration::untilTomorrow && hoursToSleep < 1)
  {
    hoursToSleep += 24;
  }

  int minutesToSleep = sleepDuration == SleepDuration::fiveMinutes ? 5 : 59 - timeinfo.tm_min;
  if (minutesToSleep < 5)
  {
    minutesToSleep += 60;
  }

  int secondsToSleep = 60 - timeinfo.tm_sec;
  int sleepTime = (hoursToSleep * 60 + minutesToSleep) * 60 + secondsToSleep;

  esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
  log_d("Setup ESP32 to sleep for %d Seconds\n", sleepTime);

  // This didn't work, need pull-up resistor
  // pinMode(GPIO_NUM_0, INPUT_PULLUP);
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  // esp_sleep_enable_ext1_wakeup(1 << GPIO_NUM_0, ESP_EXT1_WAKEUP_ANY_LOW);

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
      gfx->printCentredText("WiFi Connected");
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
    enterDeepSleep(SleepDuration::untilNextHour);
  }
}

void setClock()
{
  // TODO: replace with ezTime
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();

  log_d("Waiting for NTP time sync: ");
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2)
  {
    delay(500);
    yield();
    nowSecs = time(nullptr);
  }

  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  log_d("Current time: %s", asctime(&timeinfo));
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

bool getJpeg(String url, Battery status, bool useCache)
{
  NetworkClientSecure client;

  client.setCACert(cloudflareRootCACert);

  HTTPClient http;
  http.addHeader("Authorization", prefs.getString("token"));
  if (useCache)
  {
    http.addHeader("If-None-Match", prefs.getString("ETag"));
  }

  http.addHeader("X-Battery-voltage", String(status.cellVoltage, 3));
  http.addHeader("X-Battery-percent", String(status.cellPercent, 1));
  http.addHeader("X-Battery-chargeRate", String(status.chargeRate, 1));

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

    // TODO: if the code is 302 or something else indicating wrong token, clear the wifi and token

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

Battery getBatteryStatus()
{
  float cellPercent = 0;
  float cellVoltage = -1;
  float chargeRate = 0;
  if (maxlipo.begin())
  {
    delay(500);
    cellPercent = maxlipo.cellPercent();
    cellVoltage = maxlipo.cellVoltage();
    chargeRate = maxlipo.chargeRate();

    if (isnan(cellPercent) || isnan(cellVoltage) || isnan(chargeRate))
    {
      log_d("Failed to read cell voltage, check battery is connected!");
      cellVoltage = -2;
    }
  }

  return Battery{
      cellPercent = cellPercent,
      cellVoltage = cellVoltage,
      chargeRate = chargeRate};
}

void setup()
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER)
  {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }

  Serial.begin();

  Battery status = getBatteryStatus();

  ///////////// PREFS ////////////////
  prefs.begin("6-color-epd");

  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  gfx = new Epaper(A5, A4, A3, A2, 800, 480);
  gfx->setRotation(1);

  ////////////////////////////////////////

  if (status.needsCharging())
  {
    gfx->fillScreen(EPD_7IN3E_WHITE);
    gfx->setFont(&FreeSans24pt7b);
    gfx->setTextColor(EPD_7IN3E_RED);
    uint16_t y = gfx->printCentredText("Battery low!");
    gfx->setFont(&FreeSans12pt7b);
    y = gfx->printCentredText("Please recharge me", y + 10);
    gfx->updateDisplay();
    enterDeepSleep(SleepDuration::untilTomorrow);
  }

  ////////////////////////////////////////

  connectToWifi(wakeup_reason);
  setClock();
  doOTA();

  bool showBatteryStatus = wakeup_reason != ESP_SLEEP_WAKEUP_TIMER || status.isCharging();

  if (getJpeg("https://6-color-epd.pages.dev/photo", status, !showBatteryStatus))
  {
    if (showBatteryStatus)
    {
      gfx->setCursor(1, 1);
      gfx->setFont();
      gfx->setTextSize(2);
      gfx->setTextColor(0b1111100000000000, 0xffff);
      gfx->fillRect(0, 0, gfx->width(), 2 * 8 + 2, 0xffff);
      gfx->printf("Battery: %.1f%% %.3fV (%.1f%%)", status.cellPercent, status.cellVoltage, status.chargeRate);

      if (status.isCharging())
      {
        gfx->setTextSize(1);
        gfx->setFont(&FreeSans24pt7b);
        gfx->printCentredText("Charging...");
      }
    }
    gfx->dither();
    gfx->updateDisplay();
  }

  enterDeepSleep(status.isCharging() ? SleepDuration::untilNextHour : showBatteryStatus ? SleepDuration::fiveMinutes
                                                                                        : SleepDuration::untilTomorrow);
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);

  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
