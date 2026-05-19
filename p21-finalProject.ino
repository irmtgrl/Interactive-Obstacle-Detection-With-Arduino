#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>
#include <EEPROM.h>

#define BUTTON_PIN 2
#define ECHO_PIN 3
#define TRIGGER_PIN 4
#define IR_RECEIVER_PIN 5
#define RED_LED_PIN 9
#define YELLOW_LED_PIN 10

#define IR_BUTTON_OK 28
#define IR_BUTTON_RIGHT 90
#define IR_BUTTON_LEFT 8
#define IR_BUTTON_UP 24
#define IR_BUTTON_DOWN 82

#define DISTANCE_UNIT_CM 101
#define DISTANCE_UNIT_INCH 102
#define DISTANCE_UNIT_ROD 103
#define DISTANCE_UNIT_FEET 104
#define CM_TO_CM 1
#define CM_TO_INCH 0.3937
#define CM_TO_ROD 0.0019
#define CM_TO_FEET 0.0328
#define EEPROM_ADDRESS_DISTANCE_UNIT 201

#define OUTPUT_PINS_ARRAY_LENGTH 3
#define INPUT_PINS_ARRAY_LENGTH 2
#define DISTANCE_UNITS_ARRAY_LENGTH 4

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

//DISTANCE UNIT CHANGES
int preferedDistanceUnitIndex = 0;

int outputPins[OUTPUT_PINS_ARRAY_LENGTH] = { TRIGGER_PIN, RED_LED_PIN, YELLOW_LED_PIN };
int inputPins[INPUT_PINS_ARRAY_LENGTH] = { ECHO_PIN, BUTTON_PIN };
int distanceUnits[DISTANCE_UNITS_ARRAY_LENGTH] = { DISTANCE_UNIT_CM, DISTANCE_UNIT_INCH, DISTANCE_UNIT_ROD, DISTANCE_UNIT_FEET };
double unitMultiplier[DISTANCE_UNITS_ARRAY_LENGTH] = { CM_TO_CM, CM_TO_INCH, CM_TO_ROD, CM_TO_FEET };
String unitName[DISTANCE_UNITS_ARRAY_LENGTH] = { "CM", "INCH", "ROD", "FEET" };

void initializePinModes() {
  for(int i = 0; i < OUTPUT_PINS_ARRAY_LENGTH; i ++) {
    pinMode(outputPins[i], OUTPUT);
  }
  for(int i = 0; i < INPUT_PINS_ARRAY_LENGTH; i ++) {
    pinMode(inputPins[i], INPUT_PULLUP);
  }
}

void initializeLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Initializing...");
  delay(500);
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
    lcd.print("X in ");
    lcd.print(unitName[preferedDistanceUnitIndex]);
    lcd.print(" ");
    lcd.print(distance * unitMultiplier[preferedDistanceUnitIndex]);
    lcd.print("     ");
  }
}

void handleIRCommand(int command) {
  switch (command) {
    case IR_BUTTON_OK: {
      unlockApplication();
      break;
    } 
    case IR_BUTTON_RIGHT: {
      incrementUnitIndex();
      EEPROM.write(EEPROM_ADDRESS_DISTANCE_UNIT, preferedDistanceUnitIndex);
      break;
    }
    case IR_BUTTON_LEFT: {
      decrementtUnitIndex();
      EEPROM.write(EEPROM_ADDRESS_DISTANCE_UNIT, preferedDistanceUnitIndex);
      break;
    }
    case IR_BUTTON_UP: {
      // TO BE DETERMINED;
      break;
    }
    case IR_BUTTON_DOWN: {
      // TO BE DETERMINED;
      break;
    }
  }
}

void incrementUnitIndex() {
  preferedDistanceUnitIndex = (preferedDistanceUnitIndex + 1) % DISTANCE_UNITS_ARRAY_LENGTH; 
}

void decrementtUnitIndex() {
  preferedDistanceUnitIndex = (preferedDistanceUnitIndex - 1 + DISTANCE_UNITS_ARRAY_LENGTH) % DISTANCE_UNITS_ARRAY_LENGTH; 
}

void setup() {
  Serial.begin(115200);
  Serial.println("Interactive Obstacle Detection: On!");

  initializeLCD();
  initializePinModes();

  IrReceiver.begin(IR_RECEIVER_PIN, ENABLE_LED_FEEDBACK);

  preferedDistanceUnitIndex = EEPROM.read(EEPROM_ADDRESS_DISTANCE_UNIT);
  if(preferedDistanceUnitIndex == 255)
    preferedDistanceUnitIndex = 0;

  attachInterrupt(digitalPinToInterrupt(ECHO_PIN), detectNewDistance, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), unlockApplication, FALLING);
}

void loop() {
  unsigned long timeNow = millis();

  if(IrReceiver.decode()) {
    handleIRCommand(IrReceiver.decodedIRData.command);
    IrReceiver.resume();
  }

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
