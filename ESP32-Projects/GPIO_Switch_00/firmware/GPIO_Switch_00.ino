// --- ESP32 Internal Pull-up Example ---
// Tutorial by [ProtoCraft Electronics]

const int buttonPin = 4;
const int ledPin = 2;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);  // Internal pull-up enabled
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  int buttonState = digitalRead(buttonPin);

  if (buttonState == LOW) {        // LOW means button is PRESSED
    digitalWrite(ledPin, HIGH);
    Serial.println("Button Pressed");
    delay(200);
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("Button Released");
    delay(200);
  }
}
