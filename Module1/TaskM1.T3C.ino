const int motionSensorPin = 2;
const int pingPin = 4;
const int ledPin = 9;

volatile bool motionDetected = false;

void motionISR() {
  motionDetected = true;
}

long readUltrasonicDistance() {
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  pinMode(pingPin, INPUT);
  long duration = pulseIn(pingPin, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}

void setup() {
  pinMode(motionSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(motionSensorPin), motionISR, RISING);
  Serial.begin(9600);
}

void loop() {
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
  long distance = readUltrasonicDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  delay(500);
}
