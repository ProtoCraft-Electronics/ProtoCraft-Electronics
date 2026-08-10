/*
  ESP32 + WS2812B + fauxmoESP
  ---------------------------
  Part of the ProtoCraft Electronics YouTube channel:
  https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1

  Emulates a Philips Hue / Belkin WeMo device so Alexa can discover
  and control the strip directly on the local network — no cloud
  account required.

  Supports: on/off, brightness.
  Does NOT support: color (fauxmoESP only reports a brightness byte,
  not RGB) — this is a library limitation, not a strip limitation.
  Compare against SinricPro_LED_Alexa for full color control.

  Libraries required (Library Manager or ZIP install):
    - fauxmoESP        (github.com/vintlabs/fauxmoESP or a maintained fork)
    - FastLED
    - ESPAsyncTCP / ESPAsyncWebServer (fauxmoESP dependency)

  Known caveat: this relies on the older WeMo UDP/SSDP discovery
  protocol. Newer Echo hardware (Gen 3, Gen 4, Show) has tightened
  discovery handling and may intermittently fail to find the device.
  If discovery fails, reboot the ESP32 and re-run "Alexa, discover devices".
*/

#include <WiFi.h>
#include <fauxmoESP.h>
#include <FastLED.h>
#include "secrets.h"   // copy secrets.h.example -> secrets.h and fill in your values

// ---- Strip configuration ----
#define LED_PIN     5          // data pin to WS2812B strip
#define NUM_LEDS    32         // adjust to your strip length
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// ---- Alexa device name (what you say to Alexa) ----
#define DEVICE_NAME "desk light"

CRGB leds[NUM_LEDS];
fauxmoESP fauxmo;

bool    lightOn         = false;
uint8_t lightBrightness = 255;   // 0-255, set by fauxmo's onSetState callback

void applyLightState() {
  if (lightOn) {
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.setBrightness(lightBrightness);
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
  FastLED.show();
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
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

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  applyLightState();  // strip starts off

  setupWiFi();

  fauxmo.createServer(true);   // required for Gen3+ Echo devices to find it
  fauxmo.setPort(80);          // must be 80 for most modern Echo devices
  fauxmo.enable(true);
  fauxmo.addDevice(DEVICE_NAME);

  fauxmo.onSetState([](unsigned char device_id, const char* device_name,
                        bool state, unsigned char value) {
    Serial.printf("[fauxmo] %s -> state:%d brightness:%d\n", device_name, state, value);
    lightOn         = state;
    lightBrightness = value;
    applyLightState();
  });

  Serial.println("Say: 'Alexa, discover devices'");
}

void loop() {
  fauxmo.handle();
}
