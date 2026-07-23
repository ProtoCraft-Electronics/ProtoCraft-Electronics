/*
  ESP-NOW Unicast + SoftAP Provisioning — ProtoCraft Electronics (Part 3)
  Core: ESP32 Arduino Core v3.x

  Same 3-board unicast mesh as Part 2 — but instead of hardcoding peer MAC
  addresses and reflashing every time you want to change them, this board
  can reconfigure itself over WiFi.

  HOW IT WORKS
  ------------
  Hold BUTTON_PEER0 while powering on -> board enters CONFIG MODE:
    - Starts its own WiFi access point (SoftAP), no password (see note below)
    - Serves a web page showing this board's own MAC address and two fields
      to enter the other two boards' MAC addresses
    - Hit "Save & Reboot" -> values are written to flash (Preferences/NVS),
      the board restarts, and boots straight into normal operation using
      whatever peers you just configured

  Don't hold the button at power-on -> board boots into NORMAL MODE:
    - Loads the two peer MACs from flash (written during config mode)
    - Runs identical ESP-NOW unicast logic to Part 2: button 1 sends to
      Peer 0, button 2 sends to Peer 1, and the receive callback checks
      src_addr to light the matching LED

  SECURITY NOTE (deliberate, not an oversight):
  The config-mode access point has no password and the page is plain HTTP.
  That's fine on a bench for a demo. It would NOT be fine shipped in an
  actual product — a real device would want a WPA2 AP password at minimum,
  and ideally HTTPS or a BLE-based config flow instead of open HTTP.

  ============================================================
  EDIT THIS BLOCK FOR EACH BOARD — pin selection only.
  No MAC addresses to hardcode anymore — that's the whole point.
  ============================================================
*/

// -- Classic ESP32 Dev Kit --
#define BUTTON_PEER0 4
#define BUTTON_PEER1 13
#define LED_PEER0    18
#define LED_PEER1    19

// -- ESP32-C3 Super Mini (comment out the block above, uncomment this one instead) --
// #define BUTTON_PEER0 4
// #define BUTTON_PEER1 5
// #define LED_PEER0    6
// #define LED_PEER1    7

/*
  ============================================================
  Nothing below this line needs to change per board.
  ============================================================
*/

#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

Preferences prefs;
WebServer server(80);

uint8_t peerMac0[6];
uint8_t peerMac1[6];
bool peersConfigured = false;

// ---------- Shared helpers ----------

String macToString(const uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

bool parseMac(const String &str, uint8_t *mac) {
  int v[6];
  if (sscanf(str.c_str(), "%x:%x:%x:%x:%x:%x",
             &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t) v[i];
  return true;
}

bool loadPeerMac(const char *key, uint8_t *mac) {
  prefs.begin("espnow-cfg", true);
  size_t len = prefs.getBytes(key, mac, 6);
  prefs.end();
  return len == 6;
}

void savePeerMac(const char *key, uint8_t *mac) {
  prefs.begin("espnow-cfg", false);
  prefs.putBytes(key, mac, 6);
  prefs.end();
}

// ============================================================
// CONFIG MODE — SoftAP + web form
// ============================================================

void handleRoot() {
  uint8_t ownMac[6];
  WiFi.macAddress(ownMac);

  // Pre-fill with whatever is already saved, so reconfiguring shows current values
  uint8_t existing0[6], existing1[6];
  bool has0 = loadPeerMac("peer0", existing0);
  bool has1 = loadPeerMac("peer1", existing1);
  String val0 = has0 ? macToString(existing0) : "";
  String val1 = has1 ? macToString(existing1) : "";

  String html = "<html><head><title>ProtoCraft ESP-NOW Config</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:sans-serif;background:#0A1628;color:#E8EDF2;"
    "max-width:480px;margin:40px auto;padding:0 16px}"
    "h2{color:#4DD9AC}label{display:block;margin-top:16px;font-size:14px;color:#B8C8D8}"
    "input{width:100%;box-sizing:border-box;padding:10px;font-size:16px;margin-top:6px;"
    "border-radius:6px;border:1px solid #4DD9AC;background:#0D2040;color:#fff}"
    "button{margin-top:24px;width:100%;padding:12px;font-size:16px;font-weight:bold;"
    "background:#FF6B2B;color:#fff;border:none;border-radius:6px}"
    ".mac{font-family:monospace;color:#4DD9AC}</style></head><body>"
    "<h2>ProtoCraft ESP-NOW Setup</h2>"
    "<p>This board's MAC: <span class='mac'>" + macToString(ownMac) + "</span></p>"
    "<form action='/save' method='POST'>"
    "<label>Peer 0 MAC address</label>"
    "<input name='peer0' placeholder='AA:BB:CC:DD:EE:FF' value='" + val0 + "'>"
    "<label>Peer 1 MAC address</label>"
    "<input name='peer1' placeholder='AA:BB:CC:DD:EE:FF' value='" + val1 + "'>"
    "<button type='submit'>Save &amp; Reboot</button>"
    "</form></body></html>";

  server.send(200, "text/html", html);
}

void handleSave() {
  String p0 = server.arg("peer0");
  String p1 = server.arg("peer1");
  uint8_t m0[6], m1[6];

  if (!parseMac(p0, m0) || !parseMac(p1, m1)) {
    server.send(400, "text/html",
      "<body style='font-family:sans-serif'>"
      "Couldn't parse one of those MAC addresses — use the format "
      "AA:BB:CC:DD:EE:FF. <a href='/'>Go back</a></body>");
    return;
  }

  savePeerMac("peer0", m0);
  savePeerMac("peer1", m1);

  server.send(200, "text/html",
    "<body style='font-family:sans-serif;background:#0A1628;color:#E8EDF2;"
    "text-align:center;padding-top:60px'>"
    "<h2 style='color:#4DD9AC'>Saved.</h2>"
    "<p>Peer addresses written to flash. Rebooting into normal mode...</p>"
    "<script>setTimeout(function(){window.location='/reboot';}, 1500);</script>"
    "</body>");
}

void handleReboot() {
  server.send(200, "text/html", "Rebooting...");
  delay(300);
  ESP.restart();
}

void startConfigMode() {
  uint8_t ownMac[6];
  WiFi.macAddress(ownMac);
  String ssid = "ProtoCraft-Config-" + macToString(ownMac).substring(12); // last 2 bytes

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str());  // no password — see security note at top of file

  Serial.println("\n=== CONFIG MODE ===");
  Serial.print("Connect to WiFi network: ");
  Serial.println(ssid);
  Serial.println("Then browse to: http://192.168.4.1");

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reboot", handleReboot);
  server.begin();

  while (true) {
    server.handleClient();
  }
}

// ============================================================
// NORMAL MODE — same ESP-NOW unicast logic as Part 2
// ============================================================

typedef struct struct_message {
  bool trigger;
} struct_message;

struct_message outgoingData;

unsigned long led0OnUntil = 0;
unsigned long led1OnUntil = 0;
const unsigned long ledHoldMs = 300;

bool lastButton0State = HIGH;
bool lastButton1State = HIGH;
unsigned long debounce0 = 0, debounce1 = 0;
const unsigned long debounceDelay = 30;

void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Send OK" : "Send failed");
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  struct_message incoming;
  memcpy(&incoming, data, sizeof(incoming));
  if (!incoming.trigger) return;

  if (memcmp(info->src_addr, peerMac0, 6) == 0) {
    digitalWrite(LED_PEER0, HIGH);
    led0OnUntil = millis() + ledHoldMs;
    Serial.println("Packet from Peer 0 -> LED_PEER0 ON");
  } else if (memcmp(info->src_addr, peerMac1, 6) == 0) {
    digitalWrite(LED_PEER1, HIGH);
    led1OnUntil = millis() + ledHoldMs;
    Serial.println("Packet from Peer 1 -> LED_PEER1 ON");
  }
}

void addPeer(uint8_t *mac) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_add_peer(&peerInfo);
  }
}

void startNormalMode() {
  WiFi.mode(WIFI_STA);
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Peer 0: "); Serial.println(macToString(peerMac0));
  Serial.print("Peer 1: "); Serial.println(macToString(peerMac1));

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  addPeer(peerMac0);
  addPeer(peerMac1);

  Serial.println("Normal mode ready.");
}

// ============================================================
// setup() / loop()
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BUTTON_PEER0, INPUT_PULLUP);
  pinMode(BUTTON_PEER1, INPUT_PULLUP);
  pinMode(LED_PEER0, OUTPUT);
  pinMode(LED_PEER1, OUTPUT);
  digitalWrite(LED_PEER0, LOW);
  digitalWrite(LED_PEER1, LOW);

  delay(50); // let the button pin settle before reading it
  bool configHeld = (digitalRead(BUTTON_PEER0) == LOW);

  if (configHeld) {
    startConfigMode();  // blocks forever — only exits via reboot
    return;
  }

  peersConfigured = loadPeerMac("peer0", peerMac0) && loadPeerMac("peer1", peerMac1);

  if (!peersConfigured) {
    Serial.println("No peers configured yet.");
    Serial.println("Hold BUTTON_PEER0 at power-on to enter config mode.");
    return;
  }

  startNormalMode();
}

void loop() {
  if (!peersConfigured) {
    // Slow blink both LEDs as a "needs configuration" indicator
    bool on = (millis() / 500) % 2 == 0;
    digitalWrite(LED_PEER0, on);
    digitalWrite(LED_PEER1, on);
    return;
  }

  bool r0 = digitalRead(BUTTON_PEER0);
  if (r0 != lastButton0State) debounce0 = millis();
  if ((millis() - debounce0) > debounceDelay && r0 == LOW) {
    outgoingData.trigger = true;
    esp_now_send(peerMac0, (uint8_t *) &outgoingData, sizeof(outgoingData));
    Serial.println("Sent -> Peer 0");
    delay(200);
  }
  lastButton0State = r0;

  bool r1 = digitalRead(BUTTON_PEER1);
  if (r1 != lastButton1State) debounce1 = millis();
  if ((millis() - debounce1) > debounceDelay && r1 == LOW) {
    outgoingData.trigger = true;
    esp_now_send(peerMac1, (uint8_t *) &outgoingData, sizeof(outgoingData));
    Serial.println("Sent -> Peer 1");
    delay(200);
  }
  lastButton1State = r1;

  if (millis() > led0OnUntil) digitalWrite(LED_PEER0, LOW);
  if (millis() > led1OnUntil) digitalWrite(LED_PEER1, LOW);
}
