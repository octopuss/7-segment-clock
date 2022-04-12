#include <FastLED.h>
#include <ArduinoJson.h>
#include <CronAlarms.h>
#include "config.h"
#include "LED_Clock.h"
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

AsyncWebServer server(80);
DNSServer dns;
// needed for library

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;  // Replace with your GMT offset (seconds)
const int daylightOffset_sec = 0; // Replace with your daylight offset

AsyncWiFiManager wifiManager(&server, &dns);

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

  // exit after config instead of connecting
  wifiManager.setBreakAfterConfig(true);

  Serial.begin(115200);
  Serial.println();
  displayMessage(1);

  if (!wifiManager.autoConnect("Led-clock"))
  {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  
  IPAddress ip = WiFi.localIP();
  Serial.println("WiFi connected");
  Serial.println("IP:" + ip.toString());
  displayMessage(3);
  displayIp(ip);


  configTime(timezone, ntpServer);
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
                       displayMessage(2); });

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
                       } });

  ArduinoOTA.begin();

  Cron.create(clockUpdateSchedule, displayTime, false);


  if (owmTempEnabled == 1)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Wifi Connected");
      getWeather();
    }
    Cron.create(owmUpdateSchedule, getWeather, false);
    Cron.create(owmTempSchedule, displayWeather, false);
  }

  if (ntcTempEnabled == 1)
  {
    Cron.create(ntcTempSchedule, displayNtcTemperature, false);
  }
}


void loop()
{
  ArduinoOTA.handle();
  Cron.delay();
}
