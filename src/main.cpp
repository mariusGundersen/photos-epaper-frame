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

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10 * 60     /* Time ESP32 will go to sleep (in seconds) */

Preferences prefs;
WiFiManager wm;
uint16_t *pixels;

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

  for (int y = 0; y < h; y++)
  {
    for (int x = 0; x < w; x++)
    {
      pixels[(yOffset + y) * 600 + (xOffset + x)] = pDraw->pPixels[y * w + x];
    }
  }
  return 1;
}

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    printf("Wakeup caused by external signal using RTC_IO\n");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    printf("Wakeup caused by external signal using RTC_CNTL\n");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    printf("Wakeup caused by timer\n");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    printf("Wakeup caused by touchpad\n");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    printf("Wakeup caused by ULP program\n");
    break;
  default:
    printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
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
    printf("Failed to refresh access token, HTTP code: %d\n%s\n", httpCode, http.getString());
    return "";
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, http.getStream());

  if (error)
  {
    printf("deserialize fail\n%s\n", error.c_str());
    return "";
  }

  const char *access_token = doc["access_token"];

  printf("Got accesstoken %s\n", access_token);

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
      printf("Failed to fetch album json, HTTP code: %d\n%s\n", httpCode, http.getString());
      return "";
    }

    DeserializationError error = deserializeJson(doc, http.getStream());

    if (error)
    {
      printf("deserialize fail\n%s\n", error.c_str());
      return "";
    }

    mediaItems = doc["mediaItems"];

    printf("Got %d images of %d\n", mediaItems.size(), index);

    if (index < mediaItems.size())
      break;

    index -= mediaItems.size();
    const char *token = doc["nextPageToken"];
    pageToken = token;
    http.end();
    doc.clear();
  }

  const char *baseUrl = mediaItems[index]["baseUrl"];

  printf("Got baseUrl %s\n", baseUrl);

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
  Serial.printf("raw %d, millivolts: %d\n", raw, millivolts);
  const uint32_t upper_divider = 270;
  const uint32_t lower_divider = 560;
  return (float)(upper_divider + lower_divider) / lower_divider / 1000 * millivolts;
}

JPEGDEC jpeg;
bool getJpeg(String url, uint16_t *pixels)
{
  HTTPClient http;
  http.begin(url + "=w600-h448-c");
  int httpCode = http.GET();

  if (httpCode <= 0)
  {
    printf("Failed to fetch the JPEG image, HTTP code: %d\n", httpCode);
    return false;
  }

  int size = http.getSize();
  printf("Size: %d\n", size);
  WiFiClient *stream = http.getStreamPtr();
  uint8_t *buffer = new uint8_t[size];
  stream->readBytes(buffer, size);
  printf("downloaded image\n");
  http.end();

  if (jpeg.openRAM(buffer, size, drawImg))
  {
    printf("opened jpeg\n");
    printf("Image size: %d x %d, orientation: %d, bpp: %d\n", jpeg.getWidth(), jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());

    unsigned long lTime = micros();
    if (jpeg.decode(0, 0, 0))
    {
      lTime = micros() - lTime;
      printf("Decoded image in %d us\n", (int)lTime);
      return true;
    }
    else
    {
      printf("Failed to decode imageg %d", jpeg.getLastError());
      return false;
    }
    jpeg.close();
    delete[] buffer;
  }
  else
  {
    printf("Could not open jpeg &d\n", jpeg.getLastError());
    return false;
  }
}

GFXcanvas8 gfx = GFXcanvas8(600, 448);
void updatePictureFrame()
{

  String access_token = refreshAccessToken(prefs.getString("refresh_token"));
  if (access_token.isEmpty())
  {
    printf("Could not get access token\n");
    return;
  }

  String imageUrl = getImageUrl(prefs.getString("libraryId"), access_token, random(623));
  if (imageUrl.isEmpty())
  {
    printf("Could not get image url\n");
    return;
  }

  pixels = new uint16_t[600 * 448];
  if (!getJpeg(imageUrl, pixels))
  {
    return;
  }

  wm.disconnect();
  WiFi.mode(WIFI_OFF);

  unsigned long lTime = micros();
  floydSteinberg.dither(600, 448, pixels);
  lTime = micros() - lTime;
  printf("Dithered image in %d us\n", (int)lTime);

  lTime = micros();

  Epd epd = Epd(D5, D0, D3, D1, SCK, MISO, MOSI);
  epd.init();           // EPD init
  epd.draw_color(0x77); // Each refresh must be cleaned first

  for (int y = 0; y < 448; y++)
  {
    for (int x = 0; x < 600; x++)
    {
      gfx.drawPixel(x, y, pixels[y * 600 + x]);
    }
  }

  float voltage = getBatteryVoltage();
  int percent = (voltage - 3.3) / (4.2 - 3.3) * 100;
  Serial.printf("Battery: %d%% (%.2fv) \n", percent, voltage);
  int h = 7 + 2 + 2;
  int y = 448 - h - 1;
  int x = 4;
  int w = 9 * 6 + 2;
  gfx.drawRect(x + 0, y + 0, w + 0, h + 0, 0xffff);                     // draw border
  gfx.fillRect(x + 1, y + 1, w - 2, h - 2, percent > 20 ? 0x02 : 0x04); // draw fill, green if more than 20%, red otherwise
  gfx.drawRect(x - 2, y + h / 4, 2, h / 2, 0xffff);                     // draw tip of battery
  gfx.setCursor(x + 3, y + 2);
  gfx.setTextColor(0xffff, percent > 20 ? 0x02 : 0x04);
  gfx.setTextSize(1);
  gfx.printf("%d%% %.1fv", percent, voltage);

  epd.init();           // EPD init
  epd.draw_color(0x77); // Each refresh must be cleaned first

  epd.draw([&](int x, int y)
           { return gfx.getPixel(x, y); });

  lTime = micros() - lTime;
  printf("Transferred image in %d us\n", (int)lTime);

  // Refresh
  epd.refresh();
  epd.sleep();
}

void setup()
{
  Serial.begin(115200);

  for (int i = 0; i < 10; i++)
  {
    delay(1000);
    if (Serial)
      break;
  }

  print_wakeup_reason();
  setupBattery();
  // wm.erase();

  wm.autoConnect("e-ink", "password");

  printf("WiFi - Connected\n");

  // WiFi.mode(WIFI_MODE_APSTA);
  // WiFi.softAP("e-ink", "password");

  prefs.begin("e-ink");

  // prefs.putString("refresh_token", "...");
  // prefs.putString("libraryId", "...");

  updatePictureFrame();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  printf("Setup ESP32 to sleep for every %d Seconds\n", TIME_TO_SLEEP);

  printf("Going to sleep now\n");
  if (Serial)
    Serial.flush();

  esp_deep_sleep_start();
}

void loop()
{
}
