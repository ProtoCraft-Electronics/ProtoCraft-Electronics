/*
  ProtoCraft Electronics - Home Climate Monitor
  ESP32 + DHT11, async multi-page web dashboard.

  Sensor -> firmware -> webpage, same three-layer architecture as always,
  but the history store changed: instead of one fixed-size buffer that
  gets overwritten in place, history is now a set of small append-only
  files, one per day, in /log/. Old days get deleted outright once they
  age past the configured retention. See the README's Architecture
  section for why - short version: LittleFS is fast at appending to the
  end of a file and fast at deleting a file, but slow at overwriting the
  middle of one, and an in-place ring buffer is exactly that slow case.

  HARDWARE
    ESP32 dev board
    DHT11 sensor, data pin -> GPIO4
    If you're using a bare DHT11 (not a breakout module), add a 10k
    pull-up resistor between the data line and 3.3V.
    Push button between GPIO27 and GND (internal pull-up, no resistor
    needed). Hold 5 seconds to forget the saved WiFi network.
    Status LED on GPIO2 for button-hold feedback.

  LIBRARIES (Arduino Library Manager)
    DHT sensor library        by Adafruit
    Adafruit Unified Sensor   by Adafruit
    ESPAsyncWebServer         by ESP32Async
    AsyncTCP                  by ESP32Async
    ArduinoJson               by Benoit Blanchon (v7)
    (DNSServer, Preferences, ESPmDNS, HTTPClient, WiFiClientSecure ship
    with the ESP32 Arduino core)

  BEFORE YOU FLASH
    1. Copy secrets.h.example to secrets.h and fill in your Google Apps
       Script URL if you want cloud logging. Leave it blank to skip that
       feature entirely.
    2. Copy google_root_ca.h.example to google_root_ca.h if using cloud
       logging - see the instructions inside it.
    3. Adjust GMT_OFFSET_SEC below for your timezone if you're not in India.
    4. Upload the data/ folder to LittleFS (Tools > Upload LittleFS to
       Pico/ESP8266/ESP32).
    5. Upload this sketch. First boot opens "ProtoCraft-Setup" - connect
       with your phone and a setup page should pop up automatically.
    6. After that, the dashboard is at http://climate.local.
*/

#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "icon_bitmap.h"
#include "secrets.h"
#if __has_include("google_root_ca.h")
  #include "google_root_ca.h"
#else
  const char* GOOGLE_ROOT_CA = "";
#endif

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define RESET_PIN 27
#define STATUS_LED 2

// OLED: 1.3" 128x64, I2C, SSD1306 driver (not SH1106 - visually identical
// but not code-compatible, check yours before assuming). Default I2C pins
// on most ESP32 dev boards: SDA=GPIO21, SCL=GPIO22.
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDR 0x3C   // try 0x3D if the display stays blank
#define OLED_SDA 21
#define OLED_SCL 22
#define BOOT_SPLASH_MS 2500
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledAvailable = false;

AsyncWebServer server(80);
DNSServer dnsServer;
Preferences wifiPrefs;
Preferences logPrefs;

const long GMT_OFFSET_SEC = 5 * 3600 + 30 * 60;   // IST, change for your timezone
const int DAYLIGHT_OFFSET_SEC = 0;
const char* NTP_SERVER = "pool.ntp.org";

const unsigned long READ_INTERVAL_MS = 2000;  // DHT11 needs >=1s between reads
const char* LOG_DIR = "/log";
const char* AP_NAME = "ProtoCraft-Setup";
const size_t MAX_CHART_POINTS = 480;           // chart response is downsampled to this, regardless of retention

// Logging config, persisted, defaults on first boot
unsigned long logIntervalSec = 300;   // 5 minutes
unsigned int retentionDays = 7;
bool loggingEnabled = true;

struct HistoryPoint {
  uint32_t ts;
  int16_t tempX10;
  int16_t humX10;
};

// tier: 0 = good/comfortable, 1 = warn, 2 = bad/danger
struct AlertEvent {
  uint32_t ts;
  char metric;      // 'T' or 'H'
  int8_t fromTier;
  int8_t toTier;
  int16_t valueX10;
};

const size_t MAX_ALERT_EVENTS = 50;
AlertEvent alertEvents[MAX_ALERT_EVENTS];
size_t alertCount = 0;
size_t alertHead = 0;
const char* ALERTS_FILE = "/alerts.bin";

// Threshold config - editable from the Alerts tab, defaults match what
// shipped as hardcoded constants before this tab existed.
float tempComfortMin = 20, tempComfortMax = 26;
float tempWarnMin = 17, tempWarnMax = 35;
float humComfortMin = 40, humComfortMax = 60;
float humWarnMin = 25, humWarnMax = 95;
int8_t lastTempTier = -1;
int8_t lastHumTier = -1;
Preferences alertPrefs;

float currentTemp = NAN;
float currentHum = NAN;
unsigned long lastReadMs = 0;
unsigned long lastLogMs = 0;
unsigned long bootMillis = 0;
String currentDayFile = "";

bool provisioningMode = false;

bool buttonHeld = false;
unsigned long buttonPressStart = 0;
unsigned long lastBlinkToggle = 0;
bool ledState = false;
bool pendingForget = false;
unsigned long pendingForgetAt = 0;

// ---------- Settings persistence ----------

void loadThresholds() {
  alertPrefs.begin("alertcfg", true);
  tempComfortMin = alertPrefs.getFloat("tCMin", 20);
  tempComfortMax = alertPrefs.getFloat("tCMax", 26);
  tempWarnMin    = alertPrefs.getFloat("tWMin", 17);
  tempWarnMax    = alertPrefs.getFloat("tWMax", 35);
  humComfortMin  = alertPrefs.getFloat("hCMin", 40);
  humComfortMax  = alertPrefs.getFloat("hCMax", 60);
  humWarnMin     = alertPrefs.getFloat("hWMin", 25);
  humWarnMax     = alertPrefs.getFloat("hWMax", 95);
  alertPrefs.end();
}

void saveThresholds() {
  alertPrefs.begin("alertcfg", false);
  alertPrefs.putFloat("tCMin", tempComfortMin);
  alertPrefs.putFloat("tCMax", tempComfortMax);
  alertPrefs.putFloat("tWMin", tempWarnMin);
  alertPrefs.putFloat("tWMax", tempWarnMax);
  alertPrefs.putFloat("hCMin", humComfortMin);
  alertPrefs.putFloat("hCMax", humComfortMax);
  alertPrefs.putFloat("hWMin", humWarnMin);
  alertPrefs.putFloat("hWMax", humWarnMax);
  alertPrefs.end();
}

// 0 = good, 1 = warn, 2 = bad
int8_t tierOf(float v, float comfortMin, float comfortMax, float warnMin, float warnMax) {
  if (v >= comfortMin && v <= comfortMax) return 0;
  if (v >= warnMin && v <= warnMax) return 1;
  return 2;
}

void loadAlertsFromFlash() {
  File f = LittleFS.open(ALERTS_FILE, "r");
  if (!f) return;
  size_t count = f.size() / sizeof(AlertEvent);
  if (count > MAX_ALERT_EVENTS) count = MAX_ALERT_EVENTS;
  f.read((uint8_t*)alertEvents, count * sizeof(AlertEvent));
  f.close();
  alertCount = count;
  alertHead = count % MAX_ALERT_EVENTS;
}

void saveAlertsToFlash() {
  File f = LittleFS.open(ALERTS_FILE, "w");
  if (!f) return;
  for (size_t i = 0; i < alertCount; i++) {
    size_t idx = (alertHead + MAX_ALERT_EVENTS - alertCount + i) % MAX_ALERT_EVENTS;
    f.write((uint8_t*)&alertEvents[idx], sizeof(AlertEvent));
  }
  f.close();
}

// Alerts are rare by nature (only fire on a tier change), so writing to
// flash on every event is nowhere near the write-frequency concern that
// shaped the climate history design - this can just be simple.
void recordAlert(char metric, int8_t fromTier, int8_t toTier, float value) {
  AlertEvent e;
  e.ts = (uint32_t)time(nullptr);
  e.metric = metric;
  e.fromTier = fromTier;
  e.toTier = toTier;
  e.valueX10 = (int16_t)round(value * 10);

  alertEvents[alertHead] = e;
  alertHead = (alertHead + 1) % MAX_ALERT_EVENTS;
  if (alertCount < MAX_ALERT_EVENTS) alertCount++;
  saveAlertsToFlash();
}

// Checked on every sensor read (every 2s), independent of the logging
// interval - a tier change is worth catching quickly, not just whenever
// the next history point happens to get logged.
void checkAlertCrossings(float t, float h) {
  int8_t tempTier = tierOf(t, tempComfortMin, tempComfortMax, tempWarnMin, tempWarnMax);
  int8_t humTier = tierOf(h, humComfortMin, humComfortMax, humWarnMin, humWarnMax);

  if (lastTempTier != -1 && tempTier != lastTempTier) {
    recordAlert('T', lastTempTier, tempTier, t);
  }
  if (lastHumTier != -1 && humTier != lastHumTier) {
    recordAlert('H', lastHumTier, humTier, h);
  }
  lastTempTier = tempTier;
  lastHumTier = humTier;
}

void loadLogSettings() {
  logPrefs.begin("logcfg", true);
  logIntervalSec = logPrefs.getULong("interval", 300);
  retentionDays = logPrefs.getUInt("retention", 7);
  loggingEnabled = logPrefs.getBool("enabled", true);
  logPrefs.end();
}

void saveLogSettings() {
  logPrefs.begin("logcfg", false);
  logPrefs.putULong("interval", logIntervalSec);
  logPrefs.putUInt("retention", retentionDays);
  logPrefs.putBool("enabled", loggingEnabled);
  logPrefs.end();
}

// ---------- Day-file helpers ----------

String todayFileName() {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
  return String(LOG_DIR) + "/" + buf + ".bin";
}

// Days between two "YYYY-MM-DD" dates, using mktime so calendar math
// (month lengths, leap years) is handled for us rather than guessed at.
long daysBetween(const String& dateA, const String& dateB) {
  struct tm ta = {}, tb = {};
  sscanf(dateA.c_str(), "%d-%d-%d", &ta.tm_year, &ta.tm_mon, &ta.tm_mday);
  sscanf(dateB.c_str(), "%d-%d-%d", &tb.tm_year, &tb.tm_mon, &tb.tm_mday);
  ta.tm_year -= 1900; ta.tm_mon -= 1;
  tb.tm_year -= 1900; tb.tm_mon -= 1;
  time_t ea = mktime(&ta);
  time_t eb = mktime(&tb);
  return (long)((eb - ea) / 86400);
}

String dateFromFileName(const String& path) {
  // "/log/2026-08-22.bin" -> "2026-08-22"
  int slash = path.lastIndexOf('/');
  int dot = path.lastIndexOf('.');
  if (slash < 0 || dot < 0 || dot <= slash) return "";
  return path.substring(slash + 1, dot);
}

// Deletes any day-file older than the configured retention. Deleting a
// whole file is the fast case in LittleFS, same as appending - this is
// the "boring on purpose" cleanup step, no content editing involved.
void cleanupOldDays() {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  char todayBuf[24];
  strftime(todayBuf, sizeof(todayBuf), "%Y-%m-%d", &t);
  String today(todayBuf);

  File dir = LittleFS.open(LOG_DIR);
  if (!dir || !dir.isDirectory()) return;
  File f = dir.openNextFile();
  while (f) {
    String path = String(f.path());
    String fileDate = dateFromFileName(path);
    bool shouldDelete = false;
    if (fileDate.length() == 10) {
      long age = daysBetween(fileDate, today);
      if (age > (long)retentionDays) shouldDelete = true;
    }
    String toDelete = path;
    f.close();
    if (shouldDelete) {
      LittleFS.remove(toDelete);
      Serial.printf("[log] removed expired day file %s\n", toDelete.c_str());
    }
    f = dir.openNextFile();
  }
}

// ---------- Cloud logging ----------

bool cloudLoggingEnabled() {
  return strlen(GOOGLE_SCRIPT_URL) > 0;
}

// The one deliberately blocking call in the sketch - see the README for
// why that's fine here. Runs at whatever interval logging itself runs at.
void pushToGoogleSheets(float t, float h, uint32_t ts) {
  if (!cloudLoggingEnabled()) return;
  if (strlen(GOOGLE_ROOT_CA) == 0) {
    Serial.println("[cloud] no root CA configured, skipping (see google_root_ca.h.example)");
    return;
  }

  WiFiClientSecure client;
  client.setCACert(GOOGLE_ROOT_CA);
  HTTPClient https;
  https.setTimeout(5000);
  if (!https.begin(client, GOOGLE_SCRIPT_URL)) {
    Serial.println("[cloud] begin() failed");
    return;
  }
  https.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["ts"] = ts;
  doc["temp"] = round(t * 10) / 10.0;
  doc["hum"] = round(h * 10) / 10.0;
  String body;
  serializeJson(doc, body);

  int code = https.POST(body);
  https.end();
  Serial.println(code == 200 ? "[cloud] logged" : "[cloud] failed, HTTP " + String(code));
}

// ---------- Logging a new point ----------

void pushHistoryPoint(float t, float h) {
  if (!loggingEnabled) return;

  String fileName = todayFileName();
  bool dayChanged = (fileName != currentDayFile);
  currentDayFile = fileName;

  HistoryPoint p;
  p.ts = (uint32_t)time(nullptr);
  p.tempX10 = (int16_t)round(t * 10);
  p.humX10 = (int16_t)round(h * 10);

  File f = LittleFS.open(fileName, FILE_APPEND);
  if (!f) {
    Serial.println("[log] failed to open " + fileName + " for append");
  } else {
    f.write((uint8_t*)&p, sizeof(HistoryPoint));
    f.close();
  }

  if (dayChanged) cleanupOldDays();

  if (cloudLoggingEnabled()) pushToGoogleSheets(t, h, p.ts);
}

// ---------- WiFi: saved-credential connect + captive portal fallback ----------

bool tryConnectSavedWifi() {
  wifiPrefs.begin("wifi", true);
  String ssid = wifiPrefs.getString("ssid", "");
  String pass = wifiPrefs.getString("pass", "");
  wifiPrefs.end();
  if (ssid.length() == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("[wifi] connecting to %s", ssid.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[wifi] connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  return false;
}

void saveWifiCredentials(const String& ssid, const String& pass) {
  wifiPrefs.begin("wifi", false);
  wifiPrefs.putString("ssid", ssid);
  wifiPrefs.putString("pass", pass);
  wifiPrefs.end();
}

void forgetWifiAndRestart() {
  wifiPrefs.begin("wifi", false);
  wifiPrefs.clear();
  wifiPrefs.end();
  Serial.println("[wifi] credentials cleared, restarting into setup mode");
  delay(300);
  ESP.restart();
}

void syncTime() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  Serial.print("[time] syncing");
  time_t now = time(nullptr);
  int retries = 0;
  while (now < 8 * 3600 * 2 && retries < 20) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retries++;
  }
  Serial.println();
  if (now < 8 * 3600 * 2) Serial.println("[time] NTP sync failed");
  else Serial.println("[time] synced");
}

// ---------- Captive portal (provisioning mode) ----------

void setupProvisioningRoutes() {
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();
    if (n == -2) { WiFi.scanNetworks(true); request->send(202, "application/json", "{\"scanning\":true}"); return; }
    if (n == -1) { request->send(202, "application/json", "{\"scanning\":true}"); return; }
    JsonDocument doc;
    JsonArray arr = doc["networks"].to<JsonArray>();
    for (int i = 0; i < n; i++) {
      JsonObject net = arr.add<JsonObject>();
      net["ssid"] = WiFi.SSID(i);
      net["rssi"] = WiFi.RSSI(i);
      net["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    String out; serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true)) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing ssid\"}");
      return;
    }
    String ssid = request->getParam("ssid", true)->value();
    String pass = request->hasParam("pass", true) ? request->getParam("pass", true)->value() : "";
    saveWifiCredentials(ssid, pass);
    request->send(200, "application/json", "{\"ok\":true}");
    delay(200);
    ESP.restart();
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/setup.html", "text/html");
  });
}

void enterProvisioningMode() {
  provisioningMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_NAME);
  Serial.print("[wifi] provisioning mode, connect to \""); Serial.print(AP_NAME); Serial.println("\"");
  dnsServer.start(53, "*", WiFi.softAPIP());
  setupProvisioningRoutes();
  server.begin();
}

// ---------- Dashboard routes (normal mode) ----------

// Downsamples into a fixed number of averaged buckets, over whatever
// range is requested. This is what keeps /api/history's response small
// and Chart.js's redraw fast, whether the range is one day or the full
// retention window - the browser never sees more than MAX_CHART_POINTS
// regardless of how much raw data actually exists on flash.
//
// Only the day-files the requested range actually needs get opened -
// built directly from date arithmetic rather than listing the whole
// directory and filtering, so asking for "today" doesn't pay the cost
// of scanning thirty files to use one of them.
void handleHistory(AsyncWebServerRequest *request) {
  unsigned int viewDays = retentionDays;
  if (request->hasParam("days")) {
    int requested = request->getParam("days")->value().toInt();
    if (requested >= 1 && (unsigned int)requested <= retentionDays) viewDays = requested;
  }

  time_t now = time(nullptr);
  time_t rangeStart = now - (time_t)viewDays * 86400;
  double bucketWidth = (double)(now - rangeStart) / MAX_CHART_POINTS;
  if (bucketWidth < 1) bucketWidth = 1;

  static double sumTemp[MAX_CHART_POINTS];
  static double sumHum[MAX_CHART_POINTS];
  static uint32_t sumTs[MAX_CHART_POINTS];
  static uint16_t count[MAX_CHART_POINTS];
  for (size_t i = 0; i < MAX_CHART_POINTS; i++) { sumTemp[i]=0; sumHum[i]=0; sumTs[i]=0; count[i]=0; }

  for (unsigned int d = 0; d <= viewDays; d++) {
    time_t dayTs = now - (time_t)d * 86400;
    struct tm dt; localtime_r(&dayTs, &dt);
    char buf[24];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &dt);
    String path = String(LOG_DIR) + "/" + buf + ".bin";

    File f = LittleFS.open(path, "r");
    if (!f) continue;
    HistoryPoint p;
    while (f.readBytes((char*)&p, sizeof(HistoryPoint)) == sizeof(HistoryPoint)) {
      if ((time_t)p.ts >= rangeStart && (time_t)p.ts <= now) {
        size_t bucket = (size_t)((p.ts - rangeStart) / bucketWidth);
        if (bucket >= MAX_CHART_POINTS) bucket = MAX_CHART_POINTS - 1;
        sumTemp[bucket] += p.tempX10 / 10.0;
        sumHum[bucket] += p.humX10 / 10.0;
        sumTs[bucket] += p.ts;
        count[bucket]++;
      }
    }
    f.close();
  }

  JsonDocument doc;
  doc["interval_sec"] = logIntervalSec;
  doc["retention_days"] = retentionDays;
  doc["view_days"] = viewDays;
  JsonArray points = doc["points"].to<JsonArray>();
  for (size_t i = 0; i < MAX_CHART_POINTS; i++) {
    if (count[i] == 0) continue;
    JsonObject p = points.add<JsonObject>();
    p["ts"] = sumTs[i] / count[i];
    p["temp"] = round((sumTemp[i] / count[i]) * 10) / 10.0;
    p["hum"] = round((sumHum[i] / count[i]) * 10) / 10.0;
  }
  String out; serializeJson(doc, out);
  request->send(200, "application/json", out);
}

// Streams the full-resolution raw data as CSV, one day-file at a time,
// never holding more than one record in memory at once - unlike the
// chart endpoint, CSV export keeps full resolution since it's for the
// person's own analysis, not for redrawing a chart smoothly.
void handleExportCsv(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/csv",
    [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      static File dir;
      static File currentFile;
      static bool headerSent = false;
      static bool started = false;

      String out;
      if (!started) {
        started = true;
        dir = LittleFS.open(LOG_DIR);
      }
      if (!headerSent) {
        headerSent = true;
        out = "date,time,temp_c,humidity_pct\n";
      } else {
        while (true) {
          if (!currentFile) {
            currentFile = dir ? dir.openNextFile() : File();
            if (!currentFile) { started = false; headerSent = false; return 0; } // done
            if (currentFile.isDirectory()) { currentFile.close(); continue; }
          }
          HistoryPoint p;
          if (currentFile.readBytes((char*)&p, sizeof(HistoryPoint)) == sizeof(HistoryPoint)) {
            time_t rawTs = p.ts;
            struct tm ti; localtime_r(&rawTs, &ti);
            char dbuf[11], tbuf[9];
            strftime(dbuf, sizeof(dbuf), "%Y-%m-%d", &ti);
            strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &ti);
            out = String(dbuf) + "," + String(tbuf) + "," + String(p.tempX10/10.0,1) + "," + String(p.humX10/10.0,1) + "\n";
            break;
          } else {
            currentFile.close();
          }
        }
      }
      size_t len = out.length();
      if (len > maxLen) len = maxLen; // a single CSV line is always well under typical chunk sizes
      memcpy(buffer, out.c_str(), len);
      return len;
    });
  char filename[40];
  time_t now = time(nullptr); struct tm ti; localtime_r(&now, &ti);
  strftime(filename, sizeof(filename), "climate-history-%Y%m%d.csv", &ti);
  response->addHeader("Content-Disposition", String("attachment; filename=") + filename);
  request->send(response);
}

void setupDashboardRoutes() {
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/api/now", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    if (isnan(currentTemp) || isnan(currentHum)) { doc["temp"] = nullptr; doc["hum"] = nullptr; }
    else { doc["temp"] = round(currentTemp * 10) / 10.0; doc["hum"] = round(currentHum * 10) / 10.0; }
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = (millis() - bootMillis) / 1000;
    doc["ip"] = WiFi.localIP().toString();
    doc["hostname"] = "climate.local";
    doc["ssid"] = WiFi.SSID();
    doc["cloud_logging"] = cloudLoggingEnabled();
    doc["logging_enabled"] = loggingEnabled;
    doc["interval_sec"] = logIntervalSec;
    doc["retention_days"] = retentionDays;
    String out; serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  server.on("/api/history", HTTP_GET, handleHistory);
  server.on("/export.csv", HTTP_GET, handleExportCsv);

  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("interval_sec", true)) {
      unsigned long v = request->getParam("interval_sec", true)->value().toInt();
      if (v >= 30 && v <= 86400) logIntervalSec = v;
    }
    if (request->hasParam("retention_days", true)) {
      unsigned int v = request->getParam("retention_days", true)->value().toInt();
      if (v >= 1 && v <= 90) retentionDays = v;
    }
    if (request->hasParam("enabled", true)) {
      loggingEnabled = request->getParam("enabled", true)->value() == "true";
    }
    saveLogSettings();
    cleanupOldDays();
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/history/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
    File dir = LittleFS.open(LOG_DIR);
    if (dir && dir.isDirectory()) {
      File f = dir.openNextFile();
      while (f) {
        String path = String(f.path());
        f.close();
        LittleFS.remove(path);
        f = dir.openNextFile();
      }
    }
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/thresholds", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["temp_comfort_min"] = tempComfortMin;
    doc["temp_comfort_max"] = tempComfortMax;
    doc["temp_warn_min"] = tempWarnMin;
    doc["temp_warn_max"] = tempWarnMax;
    doc["hum_comfort_min"] = humComfortMin;
    doc["hum_comfort_max"] = humComfortMax;
    doc["hum_warn_min"] = humWarnMin;
    doc["hum_warn_max"] = humWarnMax;
    String out; serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  server.on("/api/thresholds", HTTP_POST, [](AsyncWebServerRequest *request) {
    auto setIfPresent = [request](const char* name, float& target) {
      if (request->hasParam(name, true)) target = request->getParam(name, true)->value().toFloat();
    };
    setIfPresent("temp_comfort_min", tempComfortMin);
    setIfPresent("temp_comfort_max", tempComfortMax);
    setIfPresent("temp_warn_min", tempWarnMin);
    setIfPresent("temp_warn_max", tempWarnMax);
    setIfPresent("hum_comfort_min", humComfortMin);
    setIfPresent("hum_comfort_max", humComfortMax);
    setIfPresent("hum_warn_min", humWarnMin);
    setIfPresent("hum_warn_max", humWarnMax);
    saveThresholds();
    // Thresholds changed, so whatever tier we thought we were in a moment
    // ago may no longer be accurate - force a fresh crossing check next read
    // rather than risk a stale comparison against the old boundaries.
    lastTempTier = -1;
    lastHumTier = -1;
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/alerts", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray arr = doc["events"].to<JsonArray>();
    for (size_t i = 0; i < alertCount; i++) {
      size_t idx = (alertHead + MAX_ALERT_EVENTS - alertCount + i) % MAX_ALERT_EVENTS;
      JsonObject e = arr.add<JsonObject>();
      e["ts"] = alertEvents[idx].ts;
      e["metric"] = String(alertEvents[idx].metric);
      e["from_tier"] = alertEvents[idx].fromTier;
      e["to_tier"] = alertEvents[idx].toTier;
      e["value"] = alertEvents[idx].valueX10 / 10.0;
    }
    String out; serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  // Change WiFi network directly from the running dashboard's Settings
  // tab - no captive portal round-trip needed. Same saved-credential
  // storage the portal uses, just a second door into it.
  server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true)) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing ssid\"}");
      return;
    }
    String ssid = request->getParam("ssid", true)->value();
    String pass = request->hasParam("pass", true) ? request->getParam("pass", true)->value() : "";
    saveWifiCredentials(ssid, pass);
    request->send(200, "application/json", "{\"ok\":true}");
    delay(200);
    ESP.restart();
  });

  server.on("/api/forget-wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"ok\":true}");
    pendingForget = true;
    pendingForgetAt = millis() + 500;
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
}

// ---------- Button (works in both modes) ----------

void handleResetButton() {
  bool pressed = (digitalRead(RESET_PIN) == LOW);
  if (pressed && !buttonHeld) { buttonHeld = true; buttonPressStart = millis(); }
  else if (!pressed && buttonHeld) { buttonHeld = false; digitalWrite(STATUS_LED, LOW); }

  if (buttonHeld) {
    unsigned long heldFor = millis() - buttonPressStart;
    if (heldFor >= 5000) {
      digitalWrite(STATUS_LED, HIGH);
      buttonHeld = false;
      forgetWifiAndRestart();
      return;
    }
    unsigned long blinkInterval = map(heldFor, 0, 5000, 500, 60);
    if (millis() - lastBlinkToggle > blinkInterval) {
      ledState = !ledState;
      digitalWrite(STATUS_LED, ledState);
      lastBlinkToggle = millis();
    }
  }
}

// ---------- OLED display (local readout, no phone/WiFi needed to check) ----------

int rssiToBars(int rssi) {
  if (rssi > -55) return 4;
  if (rssi > -65) return 3;
  if (rssi > -75) return 2;
  if (rssi > -85) return 1;
  return 0;
}

void drawSignalBars(int x, int y, int bars) {
  for (int i = 0; i < 4; i++) {
    int barHeight = 2 + i * 2;
    if (i < bars) {
      display.fillRect(x + i * 4, y + (8 - barHeight), 3, barHeight, SSD1306_WHITE);
    } else {
      display.drawRect(x + i * 4, y + (8 - barHeight), 3, barHeight, SSD1306_WHITE);
    }
  }
}

// Shown once at power-on, before WiFi even attempts to connect - the one
// place in this sketch where blocking with delay() is fine, same reasoning
// as the WiFi connect and NTP sync: a startup step, not something that
// runs while the dashboard is live.
void showBootScreen() {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.drawBitmap((SCREEN_WIDTH - LOGO_W) / 2, 4, logo_bmp, LOGO_W, LOGO_H, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  const char* wordmark = "PROTOCRAFT ELECTRONICS";
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(wordmark, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 42);
  display.print(wordmark);
  display.display();
  delay(BOOT_SPLASH_MS);
}

void showSetupModeScreen() {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("SETUP MODE");
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Connect WiFi to:");
  display.setTextSize(1);
  display.setCursor(0, 34);
  display.print(AP_NAME);
  display.setCursor(0, 50);
  display.println("Then open 192.168.4.1");
  display.display();
}

// Redrawn after every successful sensor read (same cadence as the alert
// crossing check), not on a separate timer - one update trigger to keep
// track of, not two.
void updateStatusScreen() {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("CLIMATE MONITOR");
  drawSignalBars(SCREEN_WIDTH - 18, 0, rssiToBars(WiFi.RSSI()));
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  if (isnan(currentTemp) || isnan(currentHum)) {
    display.setCursor(0, 28);
    display.println("Waiting for sensor...");
  } else {
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.printf("%.1f", currentTemp);
    display.setTextSize(1);
    display.print(" C");

    display.setTextSize(2);
    display.setCursor(0, 40);
    display.printf("%d", (int)round(currentHum));
    display.setTextSize(1);
    display.print(" %RH");
  }

  display.drawLine(0, 56, SCREEN_WIDTH, 56, SSD1306_WHITE);
  display.setCursor(0, 57);
  display.print("climate.local");
  display.display();
}

// ---------- Setup / loop ----------

void setup() {
  Serial.begin(115200);
  bootMillis = millis();

  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    oledAvailable = true;
    showBootScreen();
  } else {
    Serial.println("[oled] not found at 0x3C - check wiring, or try 0x3D (see OLED_I2C_ADDR)");
  }

  dht.begin();

  if (!LittleFS.begin(true)) {
    Serial.println("[fs] LittleFS mount failed");
  } else if (!LittleFS.exists(LOG_DIR)) {
    LittleFS.mkdir(LOG_DIR);
  }

  loadLogSettings();
  loadThresholds();
  loadAlertsFromFlash();

  bool connected = tryConnectSavedWifi();
  if (!connected) { enterProvisioningMode(); showSetupModeScreen(); return; }

  syncTime();
  currentDayFile = todayFileName();
  cleanupOldDays();

  if (MDNS.begin("climate")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mdns] ready at http://climate.local");
  } else {
    Serial.println("[mdns] setup failed, use the IP address instead");
  }

  setupDashboardRoutes();
  server.begin();
  Serial.println("[server] dashboard ready");

  lastReadMs = millis() - READ_INTERVAL_MS;
  lastLogMs = millis() - (logIntervalSec * 1000UL);
}

void loop() {
  handleResetButton();

  if (pendingForget && millis() >= pendingForgetAt) {
    forgetWifiAndRestart();
  }

  if (provisioningMode) {
    dnsServer.processNextRequest();
    return;
  }

  unsigned long now = millis();

  if (now - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) { currentTemp = t; currentHum = h; checkAlertCrossings(t, h); updateStatusScreen(); }
    else Serial.println("[dht11] read failed, keeping last good value");
  }

  if (now - lastLogMs >= logIntervalSec * 1000UL && !isnan(currentTemp)) {
    lastLogMs = now;
    pushHistoryPoint(currentTemp, currentHum);
  }
}
