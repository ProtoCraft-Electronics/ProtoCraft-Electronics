/*
  ESP32 + WS2812B + SinricPro
  ---------------------------
  Part of the ProtoCraft Electronics YouTube channel:
  https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1

  Official cloud-to-cloud integration: the ESP32 opens an outbound
  WebSocket to SinricPro's cloud, which is registered as an actual
  Alexa Smart Home skill. More reliable than local UDP/SSDP emulation
  (fauxmoESP/WLED's built-in method) because it doesn't depend on
  local network discovery quirks.

  Supports: on/off, brightness, full RGB color, color temperature (approximated
  as warm/cool white blend — WS2812B has no true tunable-white channel).

  IMPORTANT: brightness, color, and color temperature only work if those
  capabilities are enabled on the device in the SinricPro portal
  (portal.sinric.pro -> your device -> edit). If only "Power" is checked
  there, Alexa never sends those directives to the ESP32 no matter what
  the firmware supports.

  Setup required BEFORE flashing:
    1. Create a free account at portal.sinric.pro
    2. Add a new device, type "Light"
    3. Copy secrets.h.example to secrets.h in this folder, and fill in
       your Wi-Fi credentials plus the Device ID, App Key, and App Secret
       (secrets.h is gitignored — it never gets committed)
    4. In the Sinric Pro Alexa skill (linked via the Alexa app),
       enable the skill and it will show up as an Alexa-controllable
       device automatically — no manual "discover devices" needed

  Libraries required (Library Manager):
    - SinricPro
    - ArduinoJson
    - WebSockets (Links2004)
    - FastLED

  Free tier: up to 3 devices, no cost. Paid only if you scale beyond
  that ($3/device/year, or $39.99/year unlimited).
*/

#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProLight.h>
#include <FastLED.h>
#include "secrets.h"   // copy secrets.h.example -> secrets.h and fill in your values

// ---- Strip configuration ----
#define LED_PIN     5
#define NUM_LEDS    32
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

bool    lightOn         = true;
uint8_t lightBrightness = 100;          // percent, 0-100
CRGB    lightColor      = CRGB::White;

void applyLightState() {
  if (lightOn) {
    fill_solid(leds, NUM_LEDS, lightColor);
    FastLED.setBrightness(map(lightBrightness, 0, 100, 0, 255));
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
  FastLED.show();
}

// ---- SinricPro callbacks ----

bool onPowerState(const String &deviceId, bool &state) {
  lightOn = state;
  applyLightState();
  return true;
}

bool onBrightness(const String &deviceId, int &brightness) {
  lightBrightness = brightness;
  applyLightState();
  return true;
}

bool onColor(const String &deviceId, byte &r, byte &g, byte &b) {
  lightColor = CRGB(r, g, b);
  applyLightState();
  return true;
}

// WS2812B has no real tunable-white channel, so color temperature is
// approximated by blending toward warm-orange or cool-blue white.
CRGB kelvinToRGB(int kelvin) {
  if (kelvin <= 2700)  return CRGB(255, 169, 87);   // warm white
  if (kelvin <= 4000)  return CRGB(255, 209, 163);  // soft white
  if (kelvin <= 5000)  return CRGB(255, 236, 224);  // neutral white
  if (kelvin <= 6500)  return CRGB(255, 255, 255);  // daylight white
  return CRGB(201, 226, 255);                       // cool white
}

bool onColorTemperature(const String &deviceId, int &colorTemperature) {
  lightColor = kelvinToRGB(colorTemperature);
  applyLightState();
  return true;
}

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupSinricPro() {
  SinricProLight &myLight = SinricPro[DEVICE_ID];
  myLight.onPowerState(onPowerState);
  myLight.onBrightness(onBrightness);
  myLight.onColor(onColor);
  myLight.onColorTemperature(onColorTemperature);

  SinricPro.onConnected([]() { Serial.println("Connected to SinricPro"); });
  SinricPro.onDisconnected([]() { Serial.println("Disconnected from SinricPro"); });

  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  applyLightState();

  setupWiFi();
  setupSinricPro();
}

void loop() {
  SinricPro.handle();
}
