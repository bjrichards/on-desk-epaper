/*
 * Created on Fri Mar 31 2023
 *
 * Copyright (c) 2023 - Braeden Richards, All rights reserved.
 */

/*
 * INCLUDES
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wifi.h>

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h> // 4.2" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

// File used for containing secrets for configurations.
// For customization, create a secrets.h file in 'include/'
// directory.
// Is ignored from git control via .gitignore
//
// Definitions needed in file:
//  WIFI_SSD : <str> Name of WiFi to connect to
//  WIFI_PASSWORD : <str> Password of WiFi to connect to
#include "secrets.h"

/*
 * GLOBAL VARIABLES
 */

// Display classes. Contains configurations for e-ink display from
// GxGDEW042T2.h header file. To configure to another e-ink display,
// the appropriate header file would need to be used instead.
// Pin numbers used by ESP32 are parameters to io and display classes.
GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16);
GxEPD_Class display(io, /*RST=*/16, /*BUSY=*/4);

// Time variables
// Server to pull current UTC time from
const char *ntpServer = "pool.ntp.org";
// Offset seconds to change from UTC to localtime (PST for me)
const long gmtOffset_sec = -28800; // 8 hours in seconds
// Offset for daylight savings, if applicable in localtime.
const int daylightOffset_sec = 3600; // 1 hour

// JSON format of quotes to be used for any quote presentation onto the
// Display.
// JSON format of [{"Text":str, "Author":str},...]
char jsonQuotes[] =
    "[{\"text\":\"Genius is one percent inspiration and ninety-nine percent perspiration.\",\"author\":\"Thomas Edison\"},{\"text\":\"You can observe a lot just by watching.\",\"author\":\"Yogi Berra\"},{\"text\":\"A house divided against itself cannot stand.\",\"author\":\"Abraham Lincoln\"},{\"text\":\"Difficulties increase the nearer we get to the goal.\",\"author\":\"Johann Wolfgang von Goethe\"},{\"text\":\"Fate is in your hands and no one elses\",\"author\":\"Byron Pulsifer\"},{\"text\":\"Be the chief but never the lord.\",\"author\":\"Lao Tzu\"},{\"text\":\"Nothing happens unless first we dream.\",\"author\":\"Carl Sandburg\"},{\"text\":\"Well begun is half done.\",\"author\":\"Aristotle\"},{\"text\":\"Life is a learning experience, only if you learn.\",\"author\":\"Yogi Berra\"},{\"text\":\"Self-complacency is fatal to progress.\",\"author\":\"Margaret Sangster\"},{\"text\":\"Peace comes from within. Do not seek it without.\",\"author\":\"Buddha\"},{\"text\":\"What you give is what you get.\",\"author\":\"Byron Pulsifer\"},{\"text\":\"We can only learn to love by loving.\",\"author\":\"Iris Murdoch\"},{\"text\":\"Life is change. Growth is optional. Choose wisely.\",\"author\":\"Karen Clark\"},{\"text\":\"You'll see it when you believe it.\",\"author\":\"Wayne Dyer\"},{\"text\":\"To lead people walk behind them.\",\"author\":\"Lao Tzu\"},{\"text\":\"Having nothing, nothing can he lose.\",\"author\":\"William Shakespeare\"},{\"text\":\"Trouble is only opportunity in work clothes.\",\"author\":\"Henry J. Kaiser\"},{\"text\":\"A rolling stone gathers no moss.\",\"author\":\"Publilius Syrus\"},{\"text\":\"Ideas are the beginning points of all fortunes.\",\"author\":\"Napoleon Hill\"},{\"text\":\"Everything in life is luck.\",\"author\":\"Donald Trump\"},{\"text\":\"Doing nothing is better than being busy doing nothing.\",\"author\":\"Lao Tzu\"},{\"text\":\"Trust yourself. You know more than you think you do.\",\"author\":\"Benjamin Spock\"},{\"text\":\"Study the past, if you would divine the future.\",\"author\":\"Confucius\"},{\"text\":\"From error to error one discovers the entire truth.\",\"author\":\"Sigmund Freud\"},{\"text\":\"Well done is better than well said.\",\"author\":\"Benjamin Franklin\"},{\"text\":\"Bite off more than you can chew, then chew it.\",\"author\":\"Ella Williams\"},{\"text\":\"Work out your own salvation. Do not depend on others.\",\"author\":\"Buddha\"},{\"text\":\"One today is worth two tomorrows.\",\"author\":\"Benjamin Franklin\"},{\"text\":\"Once you choose hope, anythings possible.\",\"author\":\"Christopher Reeve\"},{\"text\":\"God always takes the simplest way.\",\"author\":\"Albert Einstein\"},{\"text\":\"One fails forward toward success.\",\"author\":\"Charles Kettering\"},{\"text\":\"Learning is a treasure that will follow its owner everywhere\",\"author\":\"Chinese proverb\"},{\"text\":\"Be as you wish to seem.\",\"author\":\"Socrates\"}]";

// Prints to display the given name and data, positioned at current cursor.
// Must supply a font to be used with this function.
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

// Prepares the e-ink display for the next round of fullscreen updates.
// Sets entire screen to WHITE and sets text color and font. Positions
// cursor to upper left corner of display.
void prepDisplay(const GFXfont *f)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
}

// Sets WiFi configuration and establishes WiFi connection. Uses
// WiFi name and password from secrets.h file.
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

// Disconnects WiFi and turns off the WiFi mode to conserve energy.
void disconnectWiFi()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// Configures time according to data pulled from the ntpServer address.
// Requires WiFi to be connected in order to access the appropriate data.
void configureTime()
{
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // Provides time for time configuration to work, as it is not immediate
  // and time will be incorrect if this is taken out
  while (time(nullptr) <= 100000)
  {
    Serial.print(".");
    delay(100);
  }
}

// Prints to the display the current time in the format of
// 'day name, month name day number year'
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

// Prints quote to display. Grabs quote from the jsonQuotes char*.
void getQuote()
{
  DynamicJsonDocument doc(24576);
  DeserializationError error = deserializeJson(doc, jsonQuotes);

  if (!error)
  {
    const char *quote = doc[0]["text"];
    display.println(quote);
  }
  else
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  }
}

// Initialization of ESP32 device. Sets Serial and Display baud rates and
// configures time.
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

// Continuous loop. Runs until ESP32 is disconnected from power.
void loop()
{
  Serial.println("Beginning Refresh...");
  Serial.println("Prepping display..");
  prepDisplay(&FreeMonoBold12pt7b);
  Serial.println("Drawing boxes..");
  display.fillRect(0, 0, 400, 300, GxEPD_BLACK);
  display.fillRect(5, 5, 390, 290, GxEPD_WHITE);
  display.fillRect(0, 26, 400, 274, GxEPD_BLACK);
  display.fillRect(5, 28, 390, 262, GxEPD_WHITE);
  Serial.println("Drawing text..");
  display.setCursor(9, 22);
  printLocalTime();
  display.setCursor(9, 64);
  display.setFont(&FreeMonoBold9pt7b);
  getQuote();
  Serial.println("Updating Display..");
  display.update();
  Serial.println("Ending Refresh...");
  display.powerDown();
  Serial.println("Refresh Ended.");
  delay(100000);
}