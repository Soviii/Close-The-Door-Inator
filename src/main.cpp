#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

// hard coding buzzer and button elements
#define BUZZER_PIN GPIO_NUM_2
#define BUTTON_PIN GPIO_NUM_36
#define DOOR_SENSOR_PIN GPIO_NUM_37
#define BUZZ_FREQUENCY 500

bool initiallyOpened = true;
int timeTillBuzz = 0;
int sensor = GPIO_NUM_36;
bool buttonPressed = false;

// for TTGO display
TFT_eSPI tft = TFT_eSPI(135, 240);
char currScreenStatus = 'G';

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
    if (digitalRead(BUTTON_PIN) == HIGH)
    {
        buttonPressed = true;
    }
}

void setup(){
    Serial.begin(9600);
    pinMode(sensor, INPUT);

    // configuring buzzer and button
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(DOOR_SENSOR_PIN, INPUT);

    tft.init();
    tft.setRotation(1);

    // setting up display on TTGO
    //  tft.init();
    //  tft.setRotation(1);
    //  tft.fillScreen(TFT_DARKGREEN);
    //  // tft.fillScreen(TFT_PINK);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    // // tft.setTextColor(TFT_BLUE);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(5);
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
    else{
        initiallyOpened = true;
        currScreenStatus = 'G';
    }

    if (currScreenStatus == 'G'){
        tft.fillScreen(TFT_GREEN);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
    }
    else if (currScreenStatus == 'Y'){
        tft.fillScreen(TFT_YELLOW);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
    } 
    else if (currScreenStatus == 'R'){
        tft.fillScreen(TFT_RED);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
    }

    // queue grace period for alarm
    if (buttonPressed){
        tft.fillScreen(TFT_BLUE);
        tft.drawString("", tft.width() / 2, tft.height() / 2 - 16);
        buttonPressed = false;
        sleep(5); // in seconds
    }

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