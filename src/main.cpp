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
#define BUZZ_FREQUENCY 2000


#define DHT20_SCL_PIN GPIO_NUM_21 // white header pin
#define DHT20_SDA_PIN GPIO_NUM_22 // blue header pin


bool initiallyOpened = true;
int timeTillBuzz = 0;
int sensor = GPIO_NUM_36;
bool buttonPressed = false;

// for TTGO display
TFT_eSPI tft = TFT_eSPI(135, 240);
char currScreenStatus = 'G';



// for connecting to E2C AWS instance 
const char hostname[] = "52.53.207.138";
char saveTemperaturePath[100] = "/save-temperature?temp=00.00";
char saveAlertPath[100] = "/save-alert";
char getReportPath[100] = "/daily-alert-report";

char ssid[] = "Sovi";    // network SSID (personal hotspot)
char pass[] = "iVoS7769"; // network password 
const uint16_t port = 5000;

const int kNetworkTimeout = 30*1000; // Number of milliseconds to wait without receiving any data before we give up
const int kNetworkDelay = 1000; // Number of milliseconds to wait if no data is available before trying again


// for temperature sensor
DHT20 dht20; // DHT object for reading temperature and sensor
float temperatureVal = 0;

void testBuzzer(){
    tone(BUZZER_PIN, BUZZ_FREQUENCY, 250);
}


void testButton(){ // low = not pressed
    if (digitalRead(BUTTON_PIN) == HIGH){
        Serial.println("BUTTON HIGH ");
    }
    else{
        Serial.println("BUTTON LOW ");
    }
}


void testDoorSensor(){
    if (digitalRead(DOOR_SENSOR_PIN) == HIGH){
        Serial.println("SENSOR HIGH ");
    }
    else{ // low = unconnected
        Serial.println("SENSOR LOW ");
    }
}



void checkFor5MinDelay(){ // low = button not pressed
    if (digitalRead(BUTTON_PIN) == HIGH){
        buttonPressed = true;
    }
}

void GatherAndPrintTemperature(){
    dht20.read();
    temperatureVal = dht20.getTemperature();
    Serial.println(temperatureVal);
    temperatureVal = temperatureVal * (9/5) + 32;
    Serial.print("Temperature: ");
    Serial.println(temperatureVal);
}


/* source code from Part A; connecting to webpage using HTTP */
void ConnectToFlaskWebPage(String API_Call){
    Serial.println(API_Call);
  WiFiClient c;
  HttpClient http(c);
  
    if(API_Call == "save-alert"){
        for(int i = 23; i < 28; i++){
            if(i == 25) continue;

            saveTemperaturePath[i] = String(temperatureVal)[i-23];
        }

        // char saveTemperaturePath[100] = "/save-temperature?temp=00.00";
    }




  
  int err = 0; // initializing error status variable

  //"52.53.207.138:5000/...",
  if (API_Call == "save-alert"){
    err = http.get(hostname, port, saveAlertPath);
  } else if (API_Call == "get-report"){
    err = http.get(hostname, port, getReportPath);
  } else if (API_Call == "save-temp"){
    err = http.get(hostname, port, saveTemperaturePath);
  }

  if (err == 0){
    Serial.println("connected to webpage");

    err = http.responseStatusCode();
    if (err >= 0){
      Serial.print("Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0){
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");

        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ( (http.connected() || http.available()) &&
               ((millis() - timeoutStart) < kNetworkTimeout) )
        {
            if (http.available())
            {
                c = http.read();
                // Print out this character
                Serial.print(c);
               
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

        Serial.println("Success!\n");
        // String response = http.responseBody();
        // Serial.println(response);

      }
      else {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else {    
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();

  Serial.println("");
}

/* connecting to wifi source; only need to do once */
void ConnectToWifi(){
  // We start by connecting to a WiFi network
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  // while the TTGO has not been connected to the wifi source
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("WiFi connected");
}


void setup(){
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
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(5);

    // ConnectToWifi();
}



void loop(){
    // testBuzzer();
    // testButton();
    // testDoorSensor();

    if (digitalRead(DOOR_SENSOR_PIN) == LOW){ // if sensor is open
        if (initiallyOpened){
            timeTillBuzz = millis() + 2000;
            initiallyOpened = false;
        }

        checkFor5MinDelay();
        currScreenStatus = 'Y';

        if (millis() > timeTillBuzz){
            testBuzzer();
            currScreenStatus = 'R';
        }
    }
    else {
        initiallyOpened = true;
        currScreenStatus = 'G';
    }

    if (currScreenStatus == 'G'){ // green for display is close
        tft.fillScreen(TFT_GREEN);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
    }
    else if (currScreenStatus == 'Y'){ // yellow for display is open but not for too long
        tft.fillScreen(TFT_YELLOW);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
    } 
    else if (currScreenStatus == 'R'){ // red for display is open and must be closed
        tft.fillScreen(TFT_RED);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
    }

    // queue grace period for alarm
    if (buttonPressed){
        tft.fillScreen(TFT_BLUE);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
        buttonPressed = false;
        ConnectToFlaskWebPage("save-alert"); // for demonstration
        sleep(5); // in seconds
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