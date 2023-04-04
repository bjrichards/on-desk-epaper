#include <Arduino.h>
#include <time.h>
#include <Wifi.h>

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h> // 4.2" b/w
// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include GxEPD_BitmapExamples

#include "secrets.h"

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/16, /*BUSY=*/4);        // arbitrary selection of (16), 4

// Time variables
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800; // PST offset
const int daylightOffset_sec = 3600;

void showData(const char name[], const char data[], const GFXfont *f)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
  display.println(name);
  display.println(data);
  display.update();
}

void prepDisplay(const GFXfont *f)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
}

// Todo: set current time on startup so this function works when checking against how long it has been
void printCurrentTime()
{
  // time(&curr_time);
  // timeinfo = localtime(&curr_time);
  // display.printf("Current time is: %s\n", asctime(timeinfo));
}

void initWiFi()
{
  WiFi.mode(WIFI_STA); // Set device as a WiFi station
  WiFi.begin(WIFI_SSD, WIFI_PASSWORD);
  Serial.print("Connection to Wifi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println(WiFi.localIP());
}

void disconnectWiFi()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void configureTime()
{
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while (time(nullptr) <= 100000)
  {
    Serial.print(".");
    delay(100);
  }
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    display.println(&timeinfo, "%A, %B %d %Y");
    display.println("Failed to obtain time.");
    return;
  }
  display.println(&timeinfo, "%A, %B %d %Y");
}

void setup()
{
  // Logging to serial port
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");

  // Initialize display
  display.init(115200);

  // Initialize WiFi and get current time
  initWiFi();
  configureTime();
  disconnectWiFi();

  Serial.println("setup done");
}

void loop()
{
  prepDisplay(&FreeMonoBold12pt7b);
  display.fillRect(0, 0, 400, 300, GxEPD_BLACK);
  display.fillRect(5, 5, 390, 290, GxEPD_WHITE);
  display.fillRect(0, 26, 400, 274, GxEPD_BLACK);
  display.fillRect(5, 28, 390, 262, GxEPD_WHITE);
  display.setCursor(9, 22);
  printLocalTime();
  display.update();
  display.powerDown();
  delay(100000);
}
