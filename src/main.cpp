#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <JPEGDEC.h>
#include <SPI.h>
#include "dither.h"
#include "Preferences.h"
#include "epd.h"
#include <secrets.h>
#include <ArduinoJson.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>
#include <Adafruit_GFX.h>
#include <driver/rtc_io.h>

// #define DEBUG 1

#ifdef DEBUG
#define DPRINT(...) printf(__VA_ARGS__)
#else
#define DPRINT(...)
#endif

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60 * 60     /* Time ESP32 will go to sleep (in seconds) */

Preferences prefs;
WiFiManager wm;
GFXcanvas16 gfx = GFXcanvas16(600, 448);

RGB palette[7] = {
    0x0000, // black
    0xffff, // white
    0x3d8e, // green
    0x5a56, // blue
    0xc268, // red
    0xff87, // yellow
    0xf569, // orange
};

FloydSteinberg floydSteinberg(7, palette);

int drawImg(JPEGDRAW *pDraw)
{
  int xOffset = pDraw->x;
  int yOffset = pDraw->y;
  int w = pDraw->iWidth;
  int h = pDraw->iHeight;

  gfx.drawRGBBitmap(xOffset, yOffset, pDraw->pPixels, w, h);
  return 1;
}

void drawGfxToEpd()
{
  unsigned long lTime = micros();

  Epd epd = Epd(D5, D0, D3, D1, SCK, MISO, MOSI);

  epd.init();           // EPD init
  epd.draw_color(0x77); // Each refresh must be cleaned first

  epd.draw([&](int x, int y)
           { return gfx.getPixel(x, y); });

  lTime = micros() - lTime;
  DPRINT("Transferred image in %d us\n", (int)lTime);

  // Refresh
  epd.refresh();
  epd.sleep();
}

void drawErrorMessage(const char *fmt, ...)
{
  gfx.fillScreen(0);
  gfx.setCursor(1, 1);
  gfx.setTextColor(0xffff, palette[1]);
  gfx.setTextSize(1);
  va_list args;
  va_start(args, fmt);
  gfx.printf(fmt, args);
  va_end(args);

  drawGfxToEpd();
}

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    DPRINT("Wakeup caused by external signal using RTC_IO\n");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    DPRINT("Wakeup caused by external signal using RTC_CNTL\n");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    DPRINT("Wakeup caused by timer\n");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    DPRINT("Wakeup caused by touchpad\n");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    DPRINT("Wakeup caused by ULP program\n");
    break;
  default:
    DPRINT("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

String refreshAccessToken(String refresh_token)
{
  HTTPClient http;

  http.useHTTP10(true);
  http.begin("https://oauth2.googleapis.com/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("client_secret=" GOOGLE_CLIENT_SECRET "&grant_type=refresh_token&refresh_token=" + refresh_token + "&client_id=" GOOGLE_CLIENT_ID);
  if (httpCode != 200)
  {
    DPRINT("Failed to refresh access token, HTTP code: %d\n%s\n", httpCode, http.getString());
    drawErrorMessage("Failed to refresh access token, HTTP code: %d\n%s\n", httpCode, http.getString());
    return "";
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, http.getStream());

  if (error)
  {
    DPRINT("deserialize fail\n%s\n", error.c_str());
    drawErrorMessage("deserialize fail\n%s\n", error.c_str());
    return "";
  }

  const char *access_token = doc["access_token"];

  DPRINT("Got accesstoken %s\n", access_token);

  http.end();
  doc.clear();
  return access_token;
}

String getImageUrl(String albumId, String access_token, int index)
{
  HTTPClient http;
  JsonDocument doc;
  String pageToken;
  JsonArray mediaItems;

  String pageSize = String(min(100, index + 1));
  while (true)
  {
    http.useHTTP10(true);
    http.begin("https://photoslibrary.googleapis.com/v1/mediaItems:search?fields=nextPageToken,mediaItems(baseUrl)");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + access_token);
    int httpCode = http.POST("{\"albumId\":\"" + albumId + "\", \"pageToken\": \"" + pageToken + "\", \"pageSize\": " + pageSize + "}");
    if (httpCode != 200)
    {
      DPRINT("Failed to fetch album json, HTTP code: %d\n%s\n", httpCode, http.getString());
      drawErrorMessage("Failed to fetch album json, HTTP code: %d\n%s\n", httpCode, http.getString());
      return "";
    }

    DeserializationError error = deserializeJson(doc, http.getStream());

    if (error)
    {
      DPRINT("deserialize fail\n%s\n", error.c_str());
      drawErrorMessage("deserialize fail\n%s\n", error.c_str());
      return "";
    }

    mediaItems = doc["mediaItems"];

    DPRINT("Got %d images of %d\n", mediaItems.size(), index);

    if (index < mediaItems.size())
      break;

    index -= mediaItems.size();
    const char *token = doc["nextPageToken"];
    pageToken = token;
    http.end();
    doc.clear();
  }

  const char *baseUrl = mediaItems[index]["baseUrl"];

  DPRINT("Got baseUrl %s\n", baseUrl);

  http.end();
  doc.clear();
  return baseUrl;
}

esp_adc_cal_characteristics_t adc_cal;

void setupBattery()
{
  // Setup battery voltage adc
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc_cal);
  adc1_config_channel_atten(ADC1_GPIO5_CHANNEL, ADC_ATTEN_DB_11);
}

float getBatteryVoltage()
{
  adc_power_acquire();
  uint32_t raw = adc1_get_raw(ADC1_GPIO5_CHANNEL);
  uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
  adc_power_release();
  DPRINT("raw %d, millivolts: %d\n", raw, millivolts);
  const float upper_divider = 270.0;
  const float lower_divider = 560.0;
  return (upper_divider + lower_divider) / lower_divider * millivolts / 1000.0;
}

JPEGDEC jpeg;
bool getJpeg(String url)
{
  HTTPClient http;
  http.begin(url + "=w600-h448-c");
  int httpCode = http.GET();

  if (httpCode <= 0)
  {
    DPRINT("Failed to fetch the JPEG image, HTTP code: %d\n", httpCode);
    drawErrorMessage("Failed to fetch the JPEG image, HTTP code: %d\n", httpCode);
    return false;
  }

  int size = http.getSize();
  DPRINT("Size: %d\n", size);
  WiFiClient *stream = http.getStreamPtr();
  uint8_t *buffer = new uint8_t[size];
  stream->readBytes(buffer, size);
  DPRINT("downloaded image\n");
  http.end();

  if (jpeg.openRAM(buffer, size, drawImg))
  {
    DPRINT("opened jpeg\n");
    DPRINT("Image size: %d x %d, orientation: %d, bpp: %d\n", jpeg.getWidth(), jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());

    unsigned long lTime = micros();
    if (jpeg.decode(0, 0, 0))
    {
      lTime = micros() - lTime;
      DPRINT("Decoded image in %d us\n", (int)lTime);
      return true;
    }
    else
    {
      DPRINT("Failed to decode imageg %d", jpeg.getLastError());
      drawErrorMessage("Failed to decode imageg %d", jpeg.getLastError());
      return false;
    }
    jpeg.close();
    delete[] buffer;
  }
  else
  {
    DPRINT("Could not open jpeg %d\n", jpeg.getLastError());
    drawErrorMessage("Could not open jpeg %d\n", jpeg.getLastError());
    return false;
  }
}

void updatePictureFrame(float voltage)
{

  String access_token = refreshAccessToken(prefs.getString("refresh_token"));
  if (access_token.isEmpty())
  {
    DPRINT("Could not get access token\n");
    drawErrorMessage("Could not get access token\n");
    return;
  }

  String imageUrl = getImageUrl(prefs.getString("libraryId"), access_token, random(623));
  if (imageUrl.isEmpty())
  {
    DPRINT("Could not get image url\n");
    drawErrorMessage("Could not get image url\n");
    return;
  }

  if (!getJpeg(imageUrl))
  {
    return;
  }

  wm.disconnect();
  WiFi.mode(WIFI_OFF);

  const float lowVolt = 3.3;
  const float highVolt = 4.2;
  int percent = (voltage - lowVolt) / (highVolt - lowVolt) * 100;
  DPRINT("Battery: %d%% (%.2fv) \n", percent, voltage);
  int h = 7 + 3 + 2;
  int y = 448 - h - 1;
  int x = 4;
  int w = 9 * 6 + 2;
  uint16_t color = percent > 20 ? palette[2] : palette[4];
  gfx.drawRect(x + 0, y + 0, w + 0, h + 0, 0xffff); // draw border
  gfx.fillRect(x + 1, y + 1, w - 2, h - 2, color);  // draw fill, green if more than 20%, red otherwise
  gfx.drawRect(x - 2, y + 3, 2, h - 6, 0xffff);     // draw tip of battery
  gfx.drawRect(x - 1, y + 4, 1, h - 8, color);
  gfx.setCursor(x + 3, y + 2);
  gfx.setTextColor(0xffff, color);
  gfx.setTextSize(1);
  gfx.printf("%d%% %.2fv", percent, voltage);

  unsigned long lTime = micros();
  floydSteinberg.dither(600, 448, gfx.getBuffer());
  lTime = micros() - lTime;
  DPRINT("Dithered image in %d us\n", (int)lTime);

  drawGfxToEpd();
}

void logBattery(float battery)
{
  HTTPClient http;
  http.begin("https://io.adafruit.com/api/v2/GundersenM/feeds/battery/data");
  http.addHeader("X-aio-key", prefs.getString("aio-key"));
  http.addHeader("content-type", "application/x-www-form-urlencoded");
  String body = "value=" + String(battery, 3);
  DPRINT("Sending body %s\n", body);
  int status = http.POST(body);
  DPRINT("Got response %d, %s\n", status, http.getString());
  http.end();
}

void setup()
{
  // rtc_gpio_deinit(GPIO_NUM_4);

#ifdef DEBUG
  Serial.begin(115200);

  for (int i = 0; i < 10; i++)
  {
    delay(1000);
    if (Serial)
      break;
  }
  delay(1000);
#endif
  print_wakeup_reason();
  setupBattery();
  float voltage = getBatteryVoltage();
  // wm.erase();

  wm.setConnectTimeout(0);
  wm.setConfigPortalTimeout(60);
  wm.autoConnect("e-ink", "password");
  // WiFi.begin();
  if (WiFi.isConnected())
  {
    DPRINT("WiFi - Connected\n");

    // WiFi.mode(WIFI_MODE_APSTA);
    // WiFi.softAP("e-ink", "password");

    prefs.begin("e-ink");

    // prefs.putString("refresh_token", "...");
    // prefs.putString("libraryId", "...");
    // prefs.putString("aio-key", "...");

    logBattery(voltage);
    updatePictureFrame(voltage);
  }
  else
  {
    drawErrorMessage("Could not connect to wifi");
  }

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  DPRINT("Setup ESP32 to sleep for every %d Seconds\n", TIME_TO_SLEEP);

  // esp_sleep_enable_ext1_wakeup(1 << GPIO_NUM_4, ESP_EXT1_WAKEUP_ANY_HIGH);

  DPRINT("Going to sleep now\n");
  if (Serial)
    Serial.flush();

  esp_deep_sleep_start();
}

void loop()
{
}
