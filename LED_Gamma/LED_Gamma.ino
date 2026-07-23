// =============================================================================
//  LED_Gamma.ino
//  ProtoCraft Electronics — youtu.be/eCLPZqBViC4
// =============================================================================
//
//  Demonstrates the visible difference between:
//    • Linear PWM fade  — raw 0–255, looks uneven to human eyes
//    • Gamma-corrected fade — adjusted for how eyes actually perceive brightness
//
//  HARDWARE
//    GPIO 25  →  220Ω  →  LED A (+)  →  GND   (Linear)
//    GPIO 26  →  220Ω  →  LED B (+)  →  GND   (Gamma corrected)
//
//  REQUIRES: ESP32 Arduino Core v3.x (Espressif)
//
// =============================================================================

#include <Arduino.h>

const int LINEAR_PIN = 25;
const int GAMMA_PIN  = 26;
const int PWM_FREQ   = 5000;  // 5 kHz — above visible flicker
const int PWM_RES    = 8;     // 8-bit: values 0–255

uint8_t gammaLUT[256];

void buildGammaLUT(float gamma = 2.2f) {
    for (int i = 0; i < 256; i++) {
        float brightness      = (float)i / 255.0f;
        float brightnessGamma = pow(brightness, gamma);
        gammaLUT[i]           = (uint8_t)((brightnessGamma * 255.0f) + 0.5f);
    }
}

void setup() {
    Serial.begin(115200);
    buildGammaLUT();

    // ESP32 Core v3.x — single call replaces ledcSetup() + ledcAttachPin()
    ledcAttach(LINEAR_PIN, PWM_FREQ, PWM_RES);
    ledcAttach(GAMMA_PIN,  PWM_FREQ, PWM_RES);

    // Print LUT to Serial — open Serial Plotter to see the gamma curve
    Serial.println("i, linear, gamma_corrected");
    for (int i = 0; i < 256; i++) {
        Serial.print(i);  Serial.print(", ");
        Serial.print(i);  Serial.print(", ");
        Serial.println(gammaLUT[i]);
    }
}

void loop() {
    // Ramp UP: 0 → full brightness
    for (int i = 0; i <= 255; i++) {
        ledcWrite(LINEAR_PIN, i);            // Raw linear value
        ledcWrite(GAMMA_PIN,  gammaLUT[i]); // Gamma-corrected value
        delay(10);
    }
    delay(500);

    // Ramp DOWN: full brightness → 0
    for (int i = 255; i >= 0; i--) {
        ledcWrite(LINEAR_PIN, i);
        ledcWrite(GAMMA_PIN,  gammaLUT[i]);
        delay(10);
    }
    delay(500);
}
