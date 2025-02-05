// Define pin numbers
const int motionSensorPin = 2; // Motion sensor input pin
const int ledPin = 9;         // Built-in LED output pin

void setup() {
  pinMode(motionSensorPin, INPUT); // Set motion sensor pin as input
  pinMode(ledPin, OUTPUT);         // Set LED pin as output
  Serial.begin(9600);              // Initialize Serial Monitor
}

void loop() {
  int motionState = digitalRead(motionSensorPin); // Read motion sensor

  if (motionState == HIGH) {
    digitalWrite(ledPin, HIGH);  // Turn LED on
    Serial.println("Motion Detected! LED ON");
  } else {
    digitalWrite(ledPin, LOW);   // Turn LED off
    Serial.println("No Motion. LED OFF");
  }

  delay(500); // Add small delay to avoid too many prints
}
