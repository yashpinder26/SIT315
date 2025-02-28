#include <PinChangeInterrupt.h>

#define TRIG_PIN 6   // Ultrasonic trigger pin
#define ECHO_PIN 2   // Ultrasonic echo pin (interrupt pin)
#define PIR_PIN 3    // PIR sensor input (replaces button, interrupt pin)
#define IR_PIN 4     // IR sensor input (PCINT)

volatile int pir_count = 0;          
volatile int ir_count = 0;
volatile int ultrasonic_count = 0;   

volatile unsigned long echo_start_time = 0;
volatile unsigned long echo_end_time = 0;
volatile int distance = 0;
volatile bool waiting_for_falling = false;

void pirISR() {
    if (digitalRead(PIR_PIN) == HIGH) {
        pir_count++;
    }
}

void irISR() {
    ir_count++;
}

void ultrasonicISR() {
    // Use a state-machine approach for the echo signal
    if (digitalRead(ECHO_PIN) == HIGH) {
        // Rising edge detected: record start time
        echo_start_time = micros();
        waiting_for_falling = true;
    } else {
        // Falling edge detected: record end time and calculate duration
        if (waiting_for_falling) {
            echo_end_time = micros();
            waiting_for_falling = false;
            unsigned long duration = echo_end_time - echo_start_time;
            distance = duration * 0.034 / 2;  // Calculate distance in cm
            if (distance < 10 && distance > 0) {
                ultrasonic_count++;
            }
        }
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(PIR_PIN, INPUT);        
    pinMode(IR_PIN, INPUT_PULLUP);
    
    // Attach interrupts:
    attachInterrupt(digitalPinToInterrupt(ECHO_PIN), ultrasonicISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIR_PIN), pirISR, RISING);  // Trigger on rising edge for PIR sensor
    attachPCINT(digitalPinToPCINT(IR_PIN), irISR, CHANGE);
}

void loop() {
    // Trigger ultrasonic sensor measurement
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Print sensor counts and distance for debugging
    Serial.print("PIR Count: ");
    Serial.print(pir_count);
    Serial.print(" | IR Count: ");
    Serial.print(ir_count);
    Serial.print(" | Ultrasonic Count: ");
    Serial.print(ultrasonic_count);
    Serial.print(" | Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    
    delay(500);
}
