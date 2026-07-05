/*
  ESP-NOW Receiver — ProtoCraft Electronics
  Board: ESP32 Dev Kit (or ESP32-C3 Super Mini)
  Core: ESP32 Arduino Core v3.x

  Listens for ESP-NOW packets from the sender and lights an LED the instant
  one arrives. No WiFi router, no IP address, no pairing beyond ESP-NOW init.
*/

#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 2

typedef struct struct_message {
  bool ledState;
} struct_message;

struct_message incomingData;

unsigned long ledOnUntil = 0;
const unsigned long ledHoldMs = 300; // how long the LED stays lit per press

// Callback: fires when a packet arrives
// Note: ESP32 Core v3.x callback signature uses esp_now_recv_info_t
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  if (incomingData.ledState) {
    digitalWrite(LED_PIN, HIGH);
    ledOnUntil = millis() + ledHoldMs;
    Serial.println("Packet received — LED ON");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready. Waiting for packets...");
}

void loop() {
  if (millis() > ledOnUntil) {
    digitalWrite(LED_PIN, LOW);
  }
}
