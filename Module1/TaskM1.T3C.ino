const int motionSensorPin = 2;
const int ultrasonicPin = 3;

volatile bool motionDetected = false;
volatile bool echoReceived = false;
volatile unsigned long startTime = 0;
volatile unsigned long travelTime = 0;

void motionISR() {
  motionDetected = true;
}

void ultrasonicISR() {
  if (digitalRead(ultrasonicPin) == HIGH) {
    startTime = micros();
  } else {
    travelTime = micros() - startTime;
    echoReceived = true;
  }
}

void setup() {
  pinMode(motionSensorPin, INPUT);
  pinMode(ultrasonicPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(motionSensorPin), motionISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ultrasonicPin), ultrasonicISR, CHANGE);

  Serial.begin(9600);
}

void loop() {
  pinMode(ultrasonicPin, OUTPUT);
  digitalWrite(ultrasonicPin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultrasonicPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(ultrasonicPin, LOW);
  pinMode(ultrasonicPin, INPUT);

  if (motionDetected) {
    Serial.println("Motion detected!");
    motionDetected = false;
  }

  if (echoReceived) {
    long distance = travelTime * 0.034 / 2;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    echoReceived = false;
  }

  delay(500);
}
