#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <DHT20.h>
#include <WiFi.h>
#include <HttpClient.h>
#include <string.h>

// hard coding buzzer and button elements
#define BUZZER_PIN GPIO_NUM_2
#define BUTTON_PIN GPIO_NUM_38
#define DOOR_SENSOR_PIN GPIO_NUM_37
#define BUZZ_FREQUENCY 1900

#define DHT20_SCL_PIN GPIO_NUM_21 // white header pin
#define DHT20_SDA_PIN GPIO_NUM_22 // blue header pin

// for door alarm functionality
bool initiallyOpened = true;
unsigned long timeTillBuzz = 0;
int sensor = GPIO_NUM_36;
bool buttonPressed = false;
bool justClosed = false;
float startOfOpenDoors;

// for TTGO display
TFT_eSPI tft = TFT_eSPI(135, 240);
char currScreenStatus = 'G';

// for connecting to E2C AWS instance
const char hostname[] = "13.57.244.20";
char saveTemperaturePath[100] = "/save-temperature?temp=00.00";
char saveAlertPath[100] = "/save-alert?time=00"; // 17, 18, 20, 21
char getReportPath[100] = "/daily-alert-report";
char dailyScore[4] = "0/5";                       // for reporting score to LCD Display
unsigned long nextTimeToSaveTemperature = 900000; // after 15 minutes
unsigned long nextTimeToDisplayReport = 10800000; // after 3 hours

char ssid[] = "<SSID>";     // network SSID (personal hotspot)
char pass[] = "<PASSWORD>"; // network password
const uint16_t port = 5000;

const int kNetworkTimeout = 30 * 1000; // Number of milliseconds to wait without receiving any data before we give up
const int kNetworkDelay = 1000;        // Number of milliseconds to wait if no data is available before trying again

// for temperature sensor
DHT20 dht20; // DHT object for reading temperature and sensor
float temperatureVal = 0;
char displayTempValues[5] = "xx F";

void soundBuzzer()
{
    tone(BUZZER_PIN, BUZZ_FREQUENCY, 250);
}

/* for testing purposes */
void testButton()
{ // low = not pressed
    if (digitalRead(BUTTON_PIN) == HIGH)
    {
        Serial.println("BUTTON HIGH ");
    }
    else
    {
        Serial.println("BUTTON LOW ");
    }
}

/*  for testing purposes */
void testDoorSensor()
{
    if (digitalRead(DOOR_SENSOR_PIN) == HIGH)
    {
        Serial.println("SENSOR HIGH ");
    }
    else
    { // low = unconnected
        Serial.println("SENSOR LOW ");
    }
}

/* checking if someone pressed the button (for refilling refrigerator) */
void checkFor5MinDelay()
{ // low = button not pressed
    if (digitalRead(BUTTON_PIN) == HIGH)
    {
        buttonPressed = true;
    }
}

/* gets new temperature values and converts them */
void GatherAndPrintTemperature()
{
    dht20.read();
    temperatureVal = dht20.getTemperature();
    // Serial.println(temperatureVal);
    temperatureVal = temperatureVal * (9 / 5) + 32;
    Serial.print("Temperature: ");
    Serial.println(temperatureVal);
    displayTempValues[0] = String(temperatureVal)[0];
    displayTempValues[1] = String(temperatureVal)[1];
}

/* source code from Part A; connecting to webpage using HTTP */
void ConnectToFlaskWebPage(String API_Call, unsigned long amountOfTimeDoorWasOpened)
{
    Serial.println(API_Call);
    WiFiClient c;
    HttpClient http(c);

    if (API_Call == "save-temp")
    {
        for (int i = 23; i < 28; i++)
        {
            if (i == 25)
                continue;

            saveTemperaturePath[i] = String(temperatureVal)[i - 23];
        }

        // char saveTemperaturePath[100] = "/save-temperature?temp=00.00";
    }
    else if (API_Call == "save-alert")
    {
        Serial.println(String(amountOfTimeDoorWasOpened));

        /* depending on number of seconds elapsed, set query parameter correctly */
        if (amountOfTimeDoorWasOpened >= 10000)
        {
            saveAlertPath[17] = String(amountOfTimeDoorWasOpened)[0];
            saveAlertPath[18] = String(amountOfTimeDoorWasOpened)[1];
        }
        else
        {
            saveAlertPath[17] = '0';
            saveAlertPath[18] = String(amountOfTimeDoorWasOpened)[0];
        }
    }


    int err = 0; // initializing error status variable



    //"13.57.244.20:5000/...",
    if (API_Call == "save-alert")
    {
        err = http.get(hostname, port, saveAlertPath);
    }
    else if (API_Call == "get-report")
    {
        err = http.get(hostname, port, getReportPath);
    }
    else if (API_Call == "save-temp")
    {
        err = http.get(hostname, port, saveTemperaturePath);
    }



    if (err == 0)
    {
        Serial.println("connected to webpage");

        err = http.responseStatusCode();
        if (err >= 0)
        {
            Serial.print("Got status code: ");
            Serial.println(err);

            // Usually you'd check that the response code is 200 or a
            // similar "success" code (200-299) before carrying on,
            // but we'll print out whatever response we get

            err = http.skipResponseHeaders();
            if (err >= 0)
            {
                int bodyLen = http.contentLength();

                /* for reading the score given to us */
                if (API_Call == "save-temp")
                {
                    // Now we've got to the body, so we can print it out
                    unsigned long timeoutStart = millis();
                    // Whilst we haven't timed out & haven't reached the end of the body
                    while ((http.connected() || http.available()) &&
                           ((millis() - timeoutStart) < kNetworkTimeout))
                    {
                        if (http.available())
                        {
                            dailyScore[0] = http.read();
                            Serial.println(dailyScore);

                            bodyLen--;
                            // We read something, reset the timeout counter
                            timeoutStart = millis();
                        }
                        else
                        {
                            // We haven't got any data, so let's pause to allow some to
                            // arrive
                            delay(kNetworkDelay);
                        }
                    }
                }

                Serial.println("Success!\n");
                // String response = http.responseBody();
                // Serial.println(response);
            }
            else
            {
                Serial.print("Failed to skip response headers: ");
                Serial.println(err);
            }
        }
        else
        {
            Serial.print("Getting response failed: ");
            Serial.println(err);
        }
    }
    else
    {
        Serial.print("Connect failed: ");
        Serial.println(err);
    }
    http.stop();

    Serial.println("");
}

/* connecting to wifi source; only need to do once */
void ConnectToWifi()
{
    // We start by connecting to a WiFi network
    delay(1000);
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, pass);

    // while the TTGO has not been connected to the wifi source
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("WiFi connected");
}

void setup()
{
    Serial.begin(9600);
    pinMode(sensor, INPUT);

    dht20.begin(DHT20_SDA_PIN, DHT20_SCL_PIN); // connecting TTGO to DHT20 sensor (for temperature and humidity) through I2C connection

    // configuring buzzer and button
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(DOOR_SENSOR_PIN, INPUT);

    tft.init();
    tft.setRotation(1);
    tft.setTextSize(3);
    tft.setTextColor(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);

    tft.fillScreen(TFT_BLACK);

    ConnectToWifi(); // connecting to wifi

    tft.fillScreen(TFT_CYAN);
    tft.drawString("Wifi good", tft.width() / 2, tft.height() / 2 - 16);
    tft.setTextSize(5);
    sleep(3);
}

void loop()
{
    // testBuzzer();
    // testButton();
    // testDoorSensor();

    if (digitalRead(DOOR_SENSOR_PIN) == LOW)
    { // if sensor is open
        if (initiallyOpened)
        {
            timeTillBuzz = millis() + 5000;
            initiallyOpened = false;
        }

        checkFor5MinDelay();
        currScreenStatus = 'Y';

        if (millis() > timeTillBuzz)
        {
            soundBuzzer();
            currScreenStatus = 'R';
            if (justClosed == false)
            { // used for future purposes when connecting to cloud
                justClosed = true;
                startOfOpenDoors = millis();
            }
        }
    }
    else if (justClosed == true)
    { // if it was red then just turned green (meaning it was closed after buzzing)
        initiallyOpened = true;
        currScreenStatus = 'G';
        unsigned long timeSpan = millis() - startOfOpenDoors;
        Serial.print("time span was: ");
        Serial.println(timeSpan);

        tft.setTextSize(3);
        tft.fillScreen(TFT_CYAN);
        tft.drawString("Uploading...", tft.width() / 2, tft.height() / 2 - 16);
        tft.setTextSize(5);
        ConnectToFlaskWebPage("save-alert", millis() - startOfOpenDoors);
        justClosed = false;
    }
    else
    { // if it was green or went from yellow-to-green
        currScreenStatus = 'G';
        initiallyOpened = true;
    }



    if (currScreenStatus == 'G')
    { // green for display is close
        tft.fillScreen(TFT_GREEN);
        tft.drawString(String(temperatureVal).substring(0, 2), tft.width() / 2, tft.height() / 2 - 16);
    }
    else if (currScreenStatus == 'Y')
    { // yellow for display is open but not for too long
        tft.fillScreen(TFT_YELLOW);
        tft.drawString(String(temperatureVal).substring(0, 2), tft.width() / 2, tft.height() / 2 - 16);
    }
    else if (currScreenStatus == 'R')
    { // red for display is open and must be closed
        tft.fillScreen(TFT_RED);
        tft.drawString(String(temperatureVal).substring(0, 2), tft.width() / 2, tft.height() / 2 - 16);
    }



    // queue grace period for alarm
    if (buttonPressed)
    {
        tft.fillScreen(TFT_BLUE);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
        buttonPressed = false;
        unsigned long loadingPhase = millis() + 300000;

        sleep(300); // in seconds; 5 minute grace period
    }

    if (millis() > nextTimeToSaveTemperature)
    { // need to save temperature after 15 minutes
        tft.setTextSize(3);
        tft.fillScreen(TFT_CYAN);
        tft.drawString("Uploading...", tft.width() / 2, tft.height() / 2 - 16);
        tft.setTextSize(5);
        ConnectToFlaskWebPage("save-temp", 0);
        nextTimeToSaveTemperature += 900000; // meaning in 15 minutes
    }

    if (millis() > nextTimeToDisplayReport)
    {
        ConnectToFlaskWebPage("get-report", 0); // need to display report after 3 hours
        tft.fillScreen(TFT_PINK);
        tft.drawString(String(dailyScore), tft.width() / 2, tft.height() / 2 - 16);
        nextTimeToDisplayReport += 10800000;
    }

    GatherAndPrintTemperature();
    delay(500);
}

/*if magnetic sensor = opened
start timer for 1 minute
check if sensor is still open
while sensor still open
beep the buzzer and flash the light
take the temperature and send it to cloud

if button is pressed call refillDisplay()

refillDisplay()
sleep for 5 min


*/