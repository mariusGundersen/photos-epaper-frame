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
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
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
    Serial.print("Failed to refresh access token, HTTP code: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
    return "";
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, http.getStream());

  if (error)
  {
    Serial.println("deserialize fail");
    Serial.println(error.c_str());
    return "";
  }

  const char *access_token = doc["access_token"];

  Serial.printf("Got accesstoken %s\n", access_token);

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
      Serial.print("Failed to fetch album json, HTTP code: ");
      Serial.println(httpCode);
      Serial.println(http.getString());
      return "";
    }

    DeserializationError error = deserializeJson(doc, http.getStream());

    if (error)
    {
      Serial.println("deserialize fail");
      Serial.println(error.c_str());
      return "";
    }

    mediaItems = doc["mediaItems"];

    Serial.printf("Got %d images of %d\n", mediaItems.size(), index);

    if (index < mediaItems.size())
      break;

    index -= mediaItems.size();
    const char *token = doc["nextPageToken"];
    pageToken = token;
    http.end();
    doc.clear();
  }

  const char *baseUrl = mediaItems[index]["baseUrl"];

  Serial.printf("Got baseUrl %s\n", baseUrl);

  http.end();
  doc.clear();
  return baseUrl;
}

JPEGDEC jpeg;
bool getJpeg(String url, uint16_t *pixels)
{
  HTTPClient http;
  http.begin(url + "=w600-h448-c");
  int httpCode = http.GET();

  if (httpCode <= 0)
  {
    Serial.print("Failed to fetch the JPEG image, HTTP code: ");
    Serial.println(httpCode);
    return false;
  }

  int size = http.getSize();
  Serial.printf("Size: %d\n", size);
  WiFiClient *stream = http.getStreamPtr();
  uint8_t *buffer = new uint8_t[size];
  stream->readBytes(buffer, size);
  Serial.println("downloaded image");
  http.end();

  if (jpeg.openRAM(buffer, size, drawImg))
  {
    Serial.println("opened jpeg");
    Serial.printf("Image size: %d x %d, orientation: %d, bpp: %d\n", jpeg.getWidth(), jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());

    unsigned long lTime = micros();
    if (jpeg.decode(0, 0, 0))
    {
      lTime = micros() - lTime;
      Serial.printf("Decoded image in %d us\n", (int)lTime);
      return true;
    }
    else
    {
      Serial.printf("Failed to decode imageg %d", jpeg.getLastError());
      return false;
    }
    jpeg.close();
    delete[] buffer;
  }
  else
  {
    Serial.printf("Could not open jpeg &d\n", jpeg.getLastError());
    return false;
  }
}

void updatePictureFrame()
{

  String access_token = refreshAccessToken(prefs.getString("refresh_token"));
  if (access_token.isEmpty())
  {
    Serial.println("Could not get access token");
    return;
  }

  String imageUrl = getImageUrl(prefs.getString("libraryId"), access_token, random(623));
  if (imageUrl.isEmpty())
  {
    Serial.println("Could not get image url");
    return;
  }
  Serial.println(imageUrl);

  pixels = new uint16_t[600 * 448];
  if (!getJpeg(imageUrl, pixels))
  {
    return;
  }

  unsigned long lTime = micros();
  floydSteinberg.dither(600, 448, pixels);
  lTime = micros() - lTime;
  Serial.printf("Dithered image in %d us\n", (int)lTime);

  lTime = micros();

  Epd epd = Epd(D5, D0, D3, D1, SCK, MISO, MOSI);
  epd.init();           // EPD init
  epd.draw_color(0x77); // Each refresh must be cleaned first

  epd.draw([&](int x, int y)
           { return pixels[y * 600 + x]; });

  lTime = micros() - lTime;
  Serial.printf("Transferred image in %d us\n", (int)lTime);

  // Refresh
  epd.refresh();
  epd.sleep();
}

void setup()
{
  Serial.begin(115200);

  delay(5000);

  print_wakeup_reason();

  // wm.erase();

  wm.autoConnect("e-ink", "password");

  Serial.println("WiFi - Connected");

  // WiFi.mode(WIFI_MODE_APSTA);
  // WiFi.softAP("e-ink", "password");

  prefs.begin("e-ink");

  // prefs.putString("refresh_token", "...");
  // prefs.putString("libraryId", "");

  updatePictureFrame();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  Serial.println("Going to sleep now");
  Serial.flush();

  esp_deep_sleep_start();
}

void loop()
{
}
