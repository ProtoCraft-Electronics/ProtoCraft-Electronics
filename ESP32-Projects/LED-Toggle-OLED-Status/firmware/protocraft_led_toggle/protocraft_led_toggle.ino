/*
 * ProtoCraft Electronics — LED Toggle with OLED Status
 * -----------------------------------------------------
 * Board:    ESP32 Dev Kit (any variant with an onboard LED)
 * Display:  1.3" OLED, SH1106 driver, I2C (128x64)
 *           (same display/library used in the LoRa display demo)
 * Switch:   Momentary push button on GPIO 4 -> GND (INPUT_PULLUP)
 *
 * Behavior:
 *   - On boot: show the ProtoCraft Electronics logo/splash for a few
 *     seconds on the OLED.
 *   - After boot: show the current LED status on the OLED.
 *   - Each press of the button toggles the onboard LED ON/OFF and
 *     updates the OLED immediately. Non-blocking (millis()-based)
 *     debounce, no delay() in the main loop.
 *
 * Libraries required (Library Manager):
 *   - Adafruit GFX Library
 *   - Adafruit SH110X
 *
 * Wiring:
 *   OLED VCC -> 3V3        OLED GND -> GND
 *   OLED SDA -> GPIO 21    OLED SCL -> GPIO 22   (ESP32 default I2C pins)
 *   Switch   -> one leg to GPIO 4, other leg to GND
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "icon_bitmap.h"   // ProtoCraft Electronics chip-icon (32x32, 1-bit) — generated from the channel logo PNG

// ---------- Display config ----------
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1      // No dedicated reset pin (shared with ESP32 reset)
#define OLED_I2C_ADDR  0x3C    // Common address for 1.3" SH1106 modules; use 0x3D if that doesn't work

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- Pin config ----------
const uint8_t LED_PIN    = LED_BUILTIN;  // Onboard LED. If your board reports no onboard LED,
                                          // set this to the actual pin, e.g. GPIO 2 on most
                                          // "DOIT ESP32 DEVKIT V1" style boards.
const uint8_t BUTTON_PIN = 4;            // Switch: GPIO4 to GND, INPUT_PULLUP (pressed = LOW)

// ---------- State ----------
bool ledState        = false;   // Current LED state (false = OFF, true = ON)
int  lastRawReading   = HIGH;   // Last raw reading of the button pin
int  debouncedState   = HIGH;   // Debounced/stable button state
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 40;  // ms

const unsigned long BOOT_SPLASH_MS = 3000; // How long to show the boot screen

// ---------- Forward declarations ----------
void showBootScreen();
void drawLedStatus();
void handleButton();

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Init OLED
  if (!display.begin(OLED_I2C_ADDR, true)) {
    Serial.println(F("SH1106 allocation failed — check wiring/I2C address"));
    // Keep going: LED toggle still works even without the display.
  } else {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    showBootScreen();
    delay(BOOT_SPLASH_MS); // One-time blocking delay is fine here — happens once at boot only
    drawLedStatus();
  }
}

void loop() {
  handleButton();
}

// Non-blocking debounce + toggle-on-press (fires once per physical press)
void handleButton() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastRawReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != debouncedState) {
      debouncedState = reading;

      // Button wired to GND with INPUT_PULLUP -> pressed = LOW
      if (debouncedState == LOW) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        drawLedStatus();
        Serial.println(ledState ? "LED: ON" : "LED: OFF");
      }
    }
  }

  lastRawReading = reading;
}

// ---------- OLED screens ----------

void showBootScreen() {
  display.clearDisplay();

  // Channel logo icon (chip outline), centered horizontally at the top
  display.drawBitmap((SCREEN_WIDTH - LOGO_W) / 2, 0, logo_bmp, LOGO_W, LOGO_H, SH110X_WHITE);

  // Wordmark below the icon
  display.setTextSize(2);
  display.setCursor(4, 34);
  display.println(F("ProtoCraft"));

  display.setTextSize(1);
  display.setCursor(1, 54);
  display.println(F("E L E C T R O N I C S"));

  display.display();
}

void drawLedStatus() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("ProtoCraft Electronics"));
  display.drawLine(0, 10, 128, 10, SH110X_WHITE);

  display.setTextSize(2);
  display.setCursor(10, 26);
  display.print(F("LED: "));
  display.println(ledState ? F("ON") : F("OFF"));

  display.setTextSize(1);
  display.setCursor(0, 54);
  display.println(ledState ? F("Press switch to turn OFF") : F("Press switch to turn ON"));

  display.display();
}
