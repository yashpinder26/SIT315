const byte LED_PIN = 13;
const byte METER_PIN = A4;

void startTimer(double timerFrequency);

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(METER_PIN, INPUT);
  Serial.begin(9600);
  double initialFrequency = 1.0;
  startTimer(initialFrequency);
}

void loop() {
  int sensorValue = analogRead(METER_PIN);
  double timerFrequency = (double)map(sensorValue, 0, 1023, 1, 10);
  Serial.print("Frequency: ");
  Serial.println(timerFrequency);
  startTimer(timerFrequency);
  delay(500);
}

void startTimer(double timerFrequency) {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  double timerInterval = 1.0 / timerFrequency;
  unsigned long timerCount = (16000000UL / 1024UL) * timerInterval - 1;
  OCR1A = (unsigned int)timerCount;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  digitalWrite(LED_PIN, digitalRead(LED_PIN) ^ 1);
}
