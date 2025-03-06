const int motionSensorPin = 2;
const int pingPin = 3;
const int ledPin = 9;

volatile bool motionDetected = false;
volatile bool echoReceived = false;
volatile unsigned long startTime = 0;
volatile unsigned long travelTime = 0;

void motionISR() {
  motionDetected = true;
}

void ultrasonicISR() {
  if (digitalRead(pingPin) == HIGH) {
    startTime = micros();
  } else {
    travelTime = micros() - startTime;
    echoReceived = true;
  }
}

void setup() {
  pinMode(motionSensorPin, INPUT);
  pinMode(pingPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(motionSensorPin), motionISR, RISING);
  attachInterrupt(digitalPinToInterrupt(pingPin), ultrasonicISR, CHANGE);

  Serial.begin(9600);
}

void loop() {
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  pinMode(pingPin, INPUT);

  if (motionDetected) {
    Serial.println("Motion detected!");
    for (int i = 0; i < 3; i++) {
      digitalWrite(ledPin, HIGH);
      delay(200);
      digitalWrite(ledPin, LOW);
      delay(200);
    }
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
