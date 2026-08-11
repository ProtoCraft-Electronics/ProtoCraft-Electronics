/*
  NUCLEO-G070RB Bring-Up Demo
  ProtoCraft Electronics

  Confirms the STM32duino toolchain end to end: reads the onboard
  user button (B1) and toggles the onboard LED (LD4), printing each
  state change over the ST-LINK virtual COM port.

  Board:  NUCLEO-G070RB (STM32G070RB, Cortex-M0+, no hardware FPU)
  Core:   STM32duino (Arduino Core for STM32)
  Note:   This board has no native USB peripheral. Both flashing and
          the Serial Monitor ride through the ST-LINK's virtual COM
          port, not a direct USB connection to the MCU.
*/

bool lastButtonState = HIGH;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(USER_BTN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println("NUCLEO-G070RB bring-up: ready.");
}

void loop() {
  bool buttonState = digitalRead(USER_BTN);

  if (buttonState != lastButtonState) {
    delay(30); // simple debounce
    buttonState = digitalRead(USER_BTN);

    if (buttonState == LOW) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      Serial.println("Button pressed. LED toggled.");
    }

    lastButtonState = buttonState;
  }
}
