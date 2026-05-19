#include <LiquidCrystal_I2C.h>

#define BUTTON_PIN 2
#define ECHO_PIN 3
#define TRIGGER_PIN 4
#define RED_LED_PIN 9
#define YELLOW_LED_PIN 10
#define OUTPUT_PINS_ARRAY_LENGTH 3
#define INPUT_PINS_ARRAY_LENGTH 2

LiquidCrystal_I2C lcd(0x27,16,2);

//ULTRASONIC SENSOR
unsigned long lastTimeSensorWasTriggered = 0;
int triggerDelay = 300;
volatile bool newDistanceDetected = false;
volatile long timeBeforeTrigger = 0;
volatile long timeAfterTrigger = 0;
double previousDistance = 400;

//YELLOW LED
int yellowLedBlinkRate = 1000;
unsigned long lastTimeYellowLedWasLit = 0;
int yellowLedState = LOW;

//APPLICATION LOCK-UNLOCK
volatile bool isApplicationLocked = false;
volatile unsigned long lastTimeButtonWasPressed = 0;
int buttonBounceDelay = 200;

int outputPins[OUTPUT_PINS_ARRAY_LENGTH] = { TRIGGER_PIN, RED_LED_PIN, YELLOW_LED_PIN };
int inputPins[INPUT_PINS_ARRAY_LENGTH] = { ECHO_PIN, BUTTON_PIN };

void initializePinModes() {
  for(int i = 0; i < OUTPUT_PINS_ARRAY_LENGTH; i ++) {
    pinMode(outputPins[i], OUTPUT);
  }
  for(int i = 0; i < INPUT_PINS_ARRAY_LENGTH; i ++) {
    pinMode(inputPins[i], INPUT_PULLUP);
  }
}

void triggerSensor() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
}

void detectNewDistance() {
  if(digitalRead(ECHO_PIN) == HIGH) {
    timeBeforeTrigger = micros();
  } else {
    timeAfterTrigger = micros();
    newDistanceDetected = true;
  }
}

double measureDistance() {
  noInterrupts();
  double time = timeAfterTrigger - timeBeforeTrigger;
  interrupts();
  double distance = time / 58.3;
  distance = previousDistance * 0.6 + distance * 0.4;
  if(distance > 400.0 || distance < 0) {
    return previousDistance;
  }
  previousDistance = distance;
  return distance;
}

void adjustLEDBlinkRate(double distance) {
  if(distance < 20) {
    yellowLedBlinkRate = 60;
  } else if(distance > 20 && distance < 50) {
    yellowLedBlinkRate = 300;
  } else {
    yellowLedBlinkRate = 1000;
  }
}

void lockApplication() {
  isApplicationLocked = true;
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, HIGH);
}

void unlockApplication() {
  unsigned long timeNow = millis(); 
  if(timeNow -  lastTimeButtonWasPressed > buttonBounceDelay) {
    lastTimeButtonWasPressed = timeNow;
    isApplicationLocked = false;
    digitalWrite(RED_LED_PIN, LOW);
  }
}

void setLCDMessage(double distance = 0.0) {
  if(isApplicationLocked) {
    lcd.setCursor(0,0);
    lcd.print("App is locked!   ");
    lcd.setCursor(0,1);
    lcd.print("Press a button.  ");
  } else {
    if(distance < 30) {
      lcd.setCursor(0,1);
      lcd.print("!! OBSTACLE !!");
    } else {
      lcd.setCursor(0,1);
      lcd.print("No obstacle.      ");
    }
    lcd.setCursor(0,0);
    lcd.print("Distance,: ");
    lcd.print(distance);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Interactive Obstacle Detection: On!");

  lcd.init();
  lcd.backlight();
  lcd.print("Hello!");

  initializePinModes();

  attachInterrupt(digitalPinToInterrupt(ECHO_PIN), detectNewDistance, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), unlockApplication, FALLING);
}

void loop() {
  unsigned long timeNow = millis();

  if(isApplicationLocked) {
    setLCDMessage();
  } else {
    if(timeNow - lastTimeSensorWasTriggered > triggerDelay) {
      lastTimeSensorWasTriggered = timeNow;
      triggerSensor();
    }

    if(newDistanceDetected) {
      newDistanceDetected = false;
      double distance = measureDistance();
      setLCDMessage(distance);
      if(distance < 10) {
        distance = 400.0;
        lockApplication();
      }
      adjustLEDBlinkRate(distance);
    }

    if(timeNow - lastTimeYellowLedWasLit > yellowLedBlinkRate) {
      yellowLedState = !yellowLedState;
      lastTimeYellowLedWasLit = timeNow;
      digitalWrite(YELLOW_LED_PIN, yellowLedState);
    }
  }
}
