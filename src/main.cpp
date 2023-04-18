#include <Arduino.h>


//hard coding buzzer and button elements
#define BUZZER_PIN GPIO_NUM_15
#define BUTTON_PIN GPIO_NUM_36
#define DOOR_SENSOR_PIN GPIO_NUM_37
#define BUZZ_FREQUENCY 500



int sensor = GPIO_NUM_36;

void testBuzzer(){
  tone(BUZZER_PIN, BUZZ_FREQUENCY, 250);
}

void testButton(){ //low = not pressed
  if(digitalRead(BUTTON_PIN) == HIGH){
    Serial.println("BUTTON HIGH "); 
  }
  else{
    Serial.println("BUTTON LOW ");
  }
}

void testDoorSensor(){
  if(digitalRead(DOOR_SENSOR_PIN) == HIGH){
      Serial.println("SENSOR HIGH "); 
    }
    else{ //low = unconnected
      Serial.println("SENSOR LOW ");
    }
}

void setup() {
  Serial.begin(9600);
  pinMode(sensor, INPUT);

  //configuring buzzer and button 
  pinMode(BUZZER_PIN, OUTPUT); 
  pinMode(BUTTON_PIN, INPUT); 
  pinMode(DOOR_SENSOR_PIN, INPUT); 
}

void loop() {
  testBuzzer(); 
  testButton();
  testDoorSensor(); 

  delay(1000);
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