// Define the digital pins: one for the button and one for the LED.
const uint8_t BTN_PIN = 2;
const uint8_t LED_PIN = 13;

// These variables keep track of the button’s previous reading and the LED’s status.
// The button defaults to HIGH because of the internal pull-up resistor.
uint8_t buttonPrevState = LOW;
uint8_t ledState = LOW;                // Start with the LED turned off.


void setup()
{
  // Set the button pin as an input with an internal pull-up resistor enabled.
  pinMode(BTN_PIN, INPUT_PULLUP);
  // Configure the LED pin to be used as an output.
  pinMode(LED_PIN, OUTPUT);
  // Start serial communication at 9600 baud for monitoring button and LED states.
  Serial.begin(9600);
}

void loop()
{
  // Read the current state of the button.
  uint8_t buttonState = digitalRead(BTN_PIN);
  
  // Display the current and previous button states along with the LED state on the serial monitor.
  Serial.print(buttonState);
  Serial.print(buttonPrevState);
  Serial.print(ledState);
  Serial.println("");
  
  
  // Detect a button press.
  if(buttonState != buttonPrevState)
  {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
  
  buttonPrevState = buttonState;
    
  // Wait for 500 milliseconds to help with debouncing and allow clear serial output.
  delay(500);
}
