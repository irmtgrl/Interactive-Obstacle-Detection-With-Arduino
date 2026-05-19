#define TRIGGER_PIN 2
#define ECHO_PIN 3
#define YELLOW_LED_PIN 10
#define OUTPUT_PINS_ARRAY_LENGTH 2

unsigned long lastTimeSensorWasTriggered;
int triggerDelay = 300;
bool newDistanceDetected = false;
long timeBeforeTrigger;
long timeAfterTrigger;
double previousDistance = 400;
double currentDistance;
int yellowLedBlinkRate = 1000;
unsigned long lastTimeYellowLedWasLit;
int yellowLedState = LOW;

int outputPins[OUTPUT_PINS_ARRAY_LENGTH] = { TRIGGER_PIN, YELLOW_LED_PIN };

void initializePinModes() {
  for(int i = 0; i < OUTPUT_PINS_ARRAY_LENGTH; i ++) {
    pinMode(outputPins[i], OUTPUT);
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
  double time = timeAfterTrigger - timeBeforeTrigger;
  double distance = time / 58.3;
  distance = previousDistance * 0.6 + distance * 0.4;
  if(distance > 400.0 || distance < 0) {
    return previousDistance;
  }
  previousDistance = distance;
  currentDistance = distance;
  return distance;
}

void adjustLEDBlinkRate() {
  if(currentDistance < 20) {
    yellowLedBlinkRate = 60;
  } else if(currentDistance > 20 && currentDistance < 50) {
    yellowLedBlinkRate = 300;
  } else {
    yellowLedBlinkRate = 1000;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Interactive Obstacle Detection: On!");

  pinMode(ECHO_PIN, INPUT);
  initializePinModes();

  attachInterrupt(digitalPinToInterrupt(ECHO_PIN), detectNewDistance, CHANGE);
}

void loop() {
  unsigned long timeNow = millis();

  if(timeNow - lastTimeSensorWasTriggered > triggerDelay) {
    lastTimeSensorWasTriggered = timeNow;
    triggerSensor();
  }

  if(newDistanceDetected) {
    double distance = measureDistance();
    Serial.println(distance);
    adjustLEDBlinkRate();
  }

  if(timeNow - lastTimeYellowLedWasLit > yellowLedBlinkRate) {
    yellowLedState = !yellowLedState;
    lastTimeYellowLedWasLit = timeNow;
    digitalWrite(YELLOW_LED_PIN, yellowLedState);
  }
}
