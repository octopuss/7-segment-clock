#if !(defined(ESP8266) || defined(ESP32))
#error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

#include <FastLED.h>
#include <ArduinoJson.h>
#include <CronAlarms.h>
#include "config.h"
#include "LED_Clock.h"
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <ArduinoOTA.h>
//needed for library


const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;  //Replace with your GMT offset (seconds)
const int daylightOffset_sec = 0; //Replace with your daylight offset

void printLocalTime()
{
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  Serial.println(asctime(timeinfo));
  delay(1000);
}

void setup()
{

  currentPalette = clockColorPalette;
  currentBlending = clockColorBlending;
  FastLED.addLeds<LED_TYPE, LED_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(ledBrightness);

  Serial.begin(115200);
  Serial.println();
  displayMessage(1);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP:" + WiFi.localIP().toString());
  WiFi.hostname(hostname);
  displayMessage(3);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  ArduinoOTA.setHostname(hostname);

  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         type = "sketch";
                       }
                       else
                       {
                         // U_FS
                         type = "filesystem";
                       }
                       // NOTE: if updating FS this would be the place to unmount FS using FS.end()
                       Serial.println("OTA start " + type);
                       displayMessage(2);
                     });

  ArduinoOTA.onEnd([]()
                   { Serial.println("OTA done"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                       {
                         Serial.println("Auth Failed");
                       }
                       else if (error == OTA_BEGIN_ERROR)
                       {
                         Serial.println("Begin Failed");
                       }
                       else if (error == OTA_CONNECT_ERROR)
                       {
                         Serial.println("Connect Failed");
                       }
                       else if (error == OTA_RECEIVE_ERROR)
                       {
                         Serial.println("Receive Failed");
                       }
                       else if (error == OTA_END_ERROR)
                       {
                         Serial.println("End Failed");
                       }
                     });

  ArduinoOTA.begin();

  //displayMessage(1);

  Cron.create(clockUpdateSchedule, displayTime, false);

  if (owmTempEnabled == 1)
  {
    getWeather();
    Cron.create(owmUpdateSchedule, getWeather, false);
    Cron.create(owmTempSchedule, displayTemperature, false);
  }
}

void loop()
{
  ArduinoOTA.handle();
  Cron.delay();
}
