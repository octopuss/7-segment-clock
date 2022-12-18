#include <WifiClient.h>
#include "FastLED_RGBW.h"
// #include <OneWire.h>
// #include <DS18B20.h>

// FastLED
#define COLOR_ORDER GRB
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *)&leds[0];
CRGBPalette16 currentPalette;
TBlendType currentBlending;
CRGB currentColor;
CRGB currentDarkColor;
uint8_t colorIndex = 0;
uint8_t charBlendIndex;
uint8_t darkBrightness = 0;
uint8_t currentBrightness = ledBrightness;
bool isNightMode = false;
const uint8_t chars = 32;

// Open Weather Map
int8_t owmTemperature = -128; // DON'T change

// Clock
int hour, minute, second;
bool secondIndicatorState = true;
uint8_t color = 0;
bool clockDisplayPaused = false;
char displayWord[5];
struct tm timeinfo;

// OneWire oneWire(D2);
// DS18B20 sensor(&oneWire);

bool getLocalTime(struct tm *info, uint32_t ms = 5000)
{
    uint32_t start = millis();
    time_t now;
    while ((millis() - start) <= ms)
    {
        time(&now);
        localtime_r(&now, info);
        if (info->tm_year > (2016 - 1900))
        {
            return true;
        }
        delay(10);
    }
    return false;
}

const uint8_t ledChar[chars][segmentsPerCharacter] = {
    {0, 1, 1, 1, 1, 1, 1}, // 0
    {0, 1, 0, 0, 0, 0, 1}, // 1, l, I
    {1, 1, 1, 0, 1, 1, 0}, // 2
    {1, 1, 1, 0, 0, 1, 1}, // 3
    {1, 1, 0, 1, 0, 0, 1}, // 4
    {1, 0, 1, 1, 0, 1, 1}, // 5 (5)
    {1, 0, 1, 1, 1, 1, 1}, // 6
    {0, 1, 1, 0, 0, 0, 1}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1}, // 9
    {1, 1, 1, 1, 1, 0, 1}, // A (10)
    {1, 0, 0, 1, 1, 1, 1}, // b
    {0, 0, 1, 1, 1, 1, 0}, // C
    {1, 0, 0, 0, 1, 1, 0}, // c
    {1, 1, 0, 0, 1, 1, 1}, // d
    {1, 0, 1, 1, 1, 1, 0}, // E (15)
    {1, 0, 1, 1, 1, 0, 0}, // F
    {1, 1, 0, 1, 1, 0, 1}, // H
    {1, 0, 0, 1, 1, 0, 1}, // h
    {0, 0, 0, 1, 1, 1, 0}, // L
    {1, 0, 0, 0, 1, 0, 1}, // n (20)
    {0, 1, 1, 1, 1, 1, 1}, // O
    {1, 0, 0, 0, 1, 1, 1}, // o
    {1, 1, 1, 1, 1, 0, 0}, // P
    {1, 0, 0, 0, 1, 0, 0}, // r
    {1, 0, 1, 1, 0, 1, 1}, // S (25)
    {0, 1, 0, 1, 1, 1, 1}, // U
    {0, 0, 0, 0, 1, 1, 1}, // u
    {1, 1, 1, 1, 0, 0, 0}, // °
    {1, 0, 0, 0, 0, 0, 0}, // -
    {1, 1, 1, 1, 0, 1, 0}, // °internal
    {0, 0, 0, 0, 0, 0, 0}, // off (30)
};

// Functions
int mapChar(char character)
{
    switch (character)
    {
    case 'A':
    case 'a':
        return 10;
        break;
    case 'B':
    case 'b':
        return 11;
        break;
    case 'C':
        return 12;
        break;
    case 'c':
        return 13;
        break;
    case 'D':
    case 'd':
        return 14;
        break;
    case 'E':
    case 'e':
        return 15;
        break;
    case 'F':
    case 'f':
        return 16;
        break;
    case 'H':
        return 17;
        break;
    case 'h':
        return 18;
        break;
    case 'I':
    case 'i':
        return 1;
        break;
    case 'L':
        return 19;
        break;
    case 'l':
        return 1;
        break;
    case 'N':
    case 'n':
        return 20;
        break;
    case 'O':
        return 21;
        break;
    case 'o':
        return 22;
        break;
    case 'P':
    case 'p':
        return 23;
        break;
    case 'R':
    case 'r':
        return 24;
        break;
    case 'S':
    case 's':
        return 25;
        break;
    case 'U':
        return 26;
        break;
    case 'u':
        return 27;
        break;
    // case '°':
    case 'z': // Workaround as "°" doesn't seem to work
        return 28;
        break;
    case '-':
        return 29;
        break;
    case 'x':
        return 30;
        break;
    default:
        return 31;
        break;
    }
}

const void fill_solid(int from, int to, CRGB color)
{
    for (int i = from; i < from + to; i++)
    {
        leds[i] = CRGBW(color.r, color.g, color.b, 0);
    }
    FastLED.show();
}

void toggleSecondIndicator()
{
    uint8_t colorCorrection = 0;
    CRGB tmpColor;
    CRGB tmpDarkColor;
    uint8_t colorMode = isNightMode ? nightClockColorMode : clockColorMode;

    darkBrightness = currentBrightness - clockSecIndicatorDiff;
    if (darkBrightness > currentBrightness)
    {
        darkBrightness = 0;
    }

    if (colorMode == 0)
    { // SOLID
        tmpColor = isNightMode ? nightClockColorSolid : clockColorSolid;
        tmpColor.fadeToBlackBy(255 - currentBrightness);
        tmpDarkColor = tmpColor;
        CHSV tempColorHsv = rgb2hsv_approximate(tmpColor);
        tempColorHsv.v = darkBrightness;
        hsv2rgb_rainbow(tempColorHsv, tmpDarkColor);
    }
    else if (colorMode == 1)
    { // PALETTE
        // Correct the color to match blend (a bit of a hack...)
        colorCorrection = 2 * clockColorCharBlend;

        tmpColor = ColorFromPalette(currentPalette, (colorIndex + colorCorrection), currentBrightness, currentBlending);
        tmpDarkColor = ColorFromPalette(currentPalette, (colorIndex + colorCorrection), darkBrightness, currentBlending);
    }

    if (secondIndicatorState)
    {
        fill_solid(NUM_LEDS - 2, 2, tmpDarkColor);
    }
    else
    {
        fill_solid(NUM_LEDS - 2, 2, tmpColor);
    }
    secondIndicatorState = !secondIndicatorState;
}

void secondIndicatorOff()
{
    fill_solid(NUM_LEDS - 2, 2, CRGB::Black);
}

void displayCharacter(uint8_t charNum, uint8_t position, bool customize, CRGBPalette16 customPalette, uint8_t customBlendIndex)
{
    if (charNum > (chars - 1))
    {
        return;
    }

    uint8_t offset = position * segmentsPerCharacter * ledsPerSegment;
    uint8_t colorMode = isNightMode ? nightClockColorMode : clockColorMode;
    for (int i = 0; i < segmentsPerCharacter; i++)
    {
        if (customize)
        {
            currentColor = ColorFromPalette(customPalette, customBlendIndex, currentBrightness, currentBlending);
        }
        else
        {
            if (colorMode == 0)
            { // SOLID
                currentColor = isNightMode ? nightClockColorSolid : clockColorSolid;
                currentColor.fadeToBlackBy(255 - currentBrightness);
            }
            else if (clockColorMode == 1)
            { // PALETTE
                currentColor = ColorFromPalette(currentPalette, charBlendIndex, currentBrightness, currentBlending);
            }
        }

        if (ledChar[charNum][i])
        {
            fill_solid(i * ledsPerSegment + offset, ledsPerSegment, currentColor);
        }
        else
        {
            fill_solid(i * ledsPerSegment + offset, ledsPerSegment, CRGB::Black);
        }
    }
}

void display(String word, bool customize = false, CRGBPalette16 customPalette = CRGBPalette16(RainbowColors_p), uint8_t customBlendIndex = 0)
{
    uint8_t charNum;
    char singleChar;
    uint8_t leadingBlanks = totalCharacters - word.length();

    for (int i = 0; i < totalCharacters; i++)
    {
        if (i < leadingBlanks)
        {
            charNum = chars - 1;
        }
        else
        {
            singleChar = word.charAt(i - leadingBlanks);
            charNum = isDigit(singleChar) ? (uint8_t)singleChar - 48 : mapChar(singleChar);

            charBlendIndex += clockColorCharBlend;
        }
        displayCharacter(charNum, i, customize, customPalette, customBlendIndex);
    }
    charBlendIndex = colorIndex;
}

void displayTime()
{
    getLocalTime(&timeinfo);

    // Stop if we don't have an acutal date yet
    if (timeinfo.tm_year < 100)
    {
        return;
    }

    if (timeinfo.tm_hour < nightStartHour && timeinfo.tm_hour >= nightEndHour)
    {
        isNightMode = false;
        currentBrightness = ledBrightness;
    }
    else
    {
        isNightMode = true;
        currentBrightness = nightLedBrightness;
    }

    uint8_t colorMode = isNightMode ? nightClockColorMode : clockColorMode;
    currentColor = ColorFromPalette(currentPalette, colorIndex, isNightMode ? nightLedBrightness : ledBrightness, currentBlending);

    // Split time into single digits
    int hourNibble10 = timeinfo.tm_hour / 10;
    int hourNibble = timeinfo.tm_hour % 10;
    int minNibble10 = timeinfo.tm_min / 10;
    int minNibble = timeinfo.tm_min % 10;

    if (hourNibble10 == 0)
    {
        sprintf(displayWord, "%d%d%d", hourNibble, minNibble10, minNibble);
        Serial.println(displayWord);
    }
    else
    {
        sprintf(displayWord, "%d%d%d%d", hourNibble10, hourNibble, minNibble10, minNibble);
    }
    display(displayWord);
    toggleSecondIndicator();

    FastLED.show();

    if (colorMode == 1)
    {
        colorIndex++;
    }
}

/* float readDSTemperature()
{
    if (sensor.begin() == false)
    {
        Serial.println("ERROR: No device found");
        while (!sensor.begin())
            ; // wait until device comes available.
    }

    sensor.setResolution(10);
    sensor.setConfig(DS18B20_CRC); // or 1
    sensor.requestTemperatures();
    // wait for data AND detect disconnect
    uint32_t timeout = millis();
    while (!sensor.isConversionComplete())
    {
        if (millis() - timeout >= 800) // check for timeout
        {
            Serial.println("ERROR: timeout or disconnect");
            break;
        }
    }

    return sensor.getTempC();
} */

int tempSamples[ntcAveragedReadingsCount];
double readNtcTemperature()
{
    uint8_t i;
    float average;

    // take N samples in a row, with a slight delay
    for (i = 0; i < ntcAveragedReadingsCount; i++)
    {
        tempSamples[i] = analogRead(ntcPin);
        delay(10);
    }

    // average all the samples out
    average = 0;
    for (i = 0; i < ntcAveragedReadingsCount; i++)
    {
        average += tempSamples[i];
    }
    average /= ntcAveragedReadingsCount;

    average = 1023 / average - 1;
    average = ntcSeriesResistor / average;
    float steinhart;
    steinhart = average / ntcResistanceNominal;   // (R/Ro)
    steinhart = log(steinhart);                   // ln(R/Ro)
    steinhart /= ntcBCoefficient;                 // 1/B * ln(R/Ro)
    steinhart += 1.0 / (ntcTempNominal + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                  // Invert
    steinhart -= 273.15;                          // convert absolute temp to C
    return steinhart + ntcTempCompensation;
}

void displayTemperature(int temperature, int minTemp, int maxTemp, String units, int displayTime, int8 internal)
{
    // Skip if we have no temperature yet
    if (temperature == -128)
    {
        return;
    }
    secondIndicatorOff();
    uint8_t customBlendIndex;

    bool negative = temperature < 0;

    if (temperature < minTemp)
    {
        customBlendIndex = 160;
    }
    else if (temperature > maxTemp)
    {
        customBlendIndex = 0;
    }
    else if (negative)
    {
        customBlendIndex = map(temperature, minTemp, (units == "metric" ? -1 : 33), 160, 127);
    }
    else
    {
        customBlendIndex = map(temperature, (units == "metric" ? 0 : 32), maxTemp, 126, 0);
    }

    int tempNibble10 = temperature / 10;
    int tempNibble = temperature % 10;
    char unit = internal == 1 ? 'x' : 'z';
    if (negative)
    {
        if (temperature <= -10)
        {
            sprintf(displayWord, "%c%d", unit, temperature);
        }
        else
        {
            sprintf(displayWord, "%d%c%c", tempNibble, unit, (units == "metric" ? 'C' : 'F'));
        }
    }
    else
    {
        sprintf(displayWord, "%d%d%c%c", tempNibble10, tempNibble, unit, (units == "metric" ? 'C' : 'F'));
    }
    Serial.println(displayWord);
    display(displayWord, true, RainbowColors_p, customBlendIndex);
    FastLED.show();
    Cron.delay(displayTime * 1000);
}

void displayNtcTemperature()
{
    double temp = readNtcTemperature();
    Serial.println(temp);
    displayTemperature(floor(temp), ntcTempMin, ntcTempMax, ntcUnits, ntcTempDisplayTime, 1);
}

void displayWeather()
{
    Serial.println(owmTemperature);
    displayTemperature(owmTemperature, owmTempMin, owmTempMax, owmUnits, owmTempDisplayTime, 0);
}

void displayMessage(uint8_t messageId)
{
    String serialMessage;
    String displayMessage;
    secondIndicatorOff();

    switch (messageId)
    {
    case 1:
        serialMessage = "Initialize";
        displayMessage = "Load";
        break;
    case 2:
        serialMessage = "Started config portal";
        displayMessage = "Conf";
        break;
    case 3:
        serialMessage = "Connecting to WiFi";
        displayMessage = "Conn";
        break;

    case 4:
        serialMessage = "Clearing";
        displayMessage = "XXXX";
        break;
    }

    Serial.println(serialMessage);
    display(displayMessage);

    FastLED.show();
}

void displayError(uint8_t errorId)
{
    String serialMessage;

    switch (errorId)
    {
    case 1:
        serialMessage = "Error 01";
        break;
    }

    int tempNibble10 = errorId / 10;
    int tempNibble = errorId % 10;

    Serial.println(serialMessage);
    sprintf(displayWord, "Er%d%d", tempNibble10, tempNibble);
    display(displayWord);

    FastLED.show();
    Cron.delay(3000);
}

void displayIp(IPAddress ip)
{
    secondIndicatorOff();
    for (int i = 0; i < 4; i++)
    {
        sprintf(displayWord, "%d", ip[i]);
        display(displayWord);
        delay(1000);
    }
}

void getWeather()
{
    WiFiClient client;
    String line = "";
    Serial.println("getWeather - Starting connection to OWM server");

    // Convert string to char array for client,connect function
    char owmApiServerChar[owmApiServer.length() + 1];
    owmApiServer.toCharArray(owmApiServerChar, owmApiServer.length() + 1);

    if (client.connect(owmApiServerChar, 80))
    {
        // Serial.println("getWeather - Connected");
        client.print("GET /data/2.5/forecast?");
        client.print("q=" + owmLocation);
        client.print("&appid=" + owmApiKey);
        client.print("&cnt=1");
        client.println("&units=" + owmUnits);
        client.println("Host: " + owmApiServer);
        client.println("Connection: close");
        client.println();
    }
    else
    {
        Serial.println("getWeater - Unable to connect");
    }
    Serial.println("connected");
    while (client.connected())
    {
        StaticJsonDocument<5000> jsonBuffer;
        line = client.readStringUntil('\n');

        Serial.println("getWeather - Parsing values");
        auto error = deserializeJson(jsonBuffer, line);
        if (error)
        {
            Serial.println("getWeather - Deserialize failed");
            return;
        }

        String owmTemperatureRaw = jsonBuffer["list"][0]["main"]["temp"];
        owmTemperature = round(owmTemperatureRaw.toFloat());
    }
}