const int motionSensorPin = 2;
const int ledPin = 9;

volatile bool motionDetected = false;

void motionISR() {
    motionDetected = true;
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
}
