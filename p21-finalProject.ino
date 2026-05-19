#define TRIGGER_PIN 2
#define ECHO_PIN 3

unsigned long lastTimeSensorWasTriggered;
int triggerDelay = 300;
bool newDistanceDetected = false;
long timeBeforeTrigger;
long timeAfterTrigger;
double previousDistance = 400;

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
  return distance;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Interactive Obstacle Detection: On!");

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

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
  }
}
