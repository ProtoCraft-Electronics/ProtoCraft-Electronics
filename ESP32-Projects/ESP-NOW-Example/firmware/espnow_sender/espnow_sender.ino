/*
  ESP-NOW Sender — ProtoCraft Electronics
  Board: ESP32 Dev Kit (or ESP32-C3 Super Mini)
  Core: ESP32 Arduino Core v3.x

  Reads a button and sends a broadcast ESP-NOW packet the instant it's pressed.
  Uses the broadcast address so you don't need to hardcode the receiver's MAC.
*/

#include <esp_now.h>
#include <WiFi.h>

#define BUTTON_PIN 4   // button to GND, using internal pull-up

// Broadcast to all ESP-NOW peers in range — no need to know the receiver's MAC
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Struct = the actual data payload sent over the air
typedef struct struct_message {
  bool ledState;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 30;

// Callback: fires after a send attempt completes
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Sender ready.");
}

void loop() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {  // button pressed (active low)
      myData.ledState = true;
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

      Serial.println(result == ESP_OK ? "Sent: LED ON" : "Send error");
      delay(200); // simple guard against repeat sends while held
    }
  }

  lastButtonState = reading;
}
