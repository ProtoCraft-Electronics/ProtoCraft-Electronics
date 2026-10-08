/*
  ESP32 Web LED Switch
  ProtoCraft Electronics

  Connects to WiFi, then serves a single page with one switch.
  Tap the switch on your phone or laptop and the LED on GPIO 2 turns on or off.

  Board:    ESP32 Dev Kit (WROOM)
  Core:     ESP32 Arduino core v3.x
  Library:  none to install (WiFi and WebServer ship with the core)
  Setup:    copy secrets.h.example to secrets.h and add your WiFi details
*/

#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"

const uint8_t LED_PIN = 2;
const uint32_t WIFI_TIMEOUT_MS = 20000;

WebServer server(80);
bool ledOn = false;

// ---------------------------------------------------------------------------
// The web page. It lives inside the sketch so there is nothing extra to upload.
// On load it asks the ESP32 for the real LED state, so a refresh never lies.
// ---------------------------------------------------------------------------
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>ESP32 Light Switch</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;500;600;700;800&display=swap" rel="stylesheet">
<style>
  :root{
    --bg-peach:#FFE8D6; --bg-violet:#E3D8FB; --bg-sky:#D2ECFB; --bg-mint:#D6F5E3;
    --blob-violet:#8B5CF6; --blob-pink:#F472B6; --blob-orange:#FB923C; --blob-teal:#2DD4BF;
    --ink:#241B3A; --ink-soft:#5B5470; --ink-faint:#9C96AD;
    --card:rgba(255,255,255,0.72); --card-border:rgba(255,255,255,0.9);
    --good:#0F9D6C; --good-bg:#D9F7EA; --bad:#C22E2E; --bad-bg:#FBDCDC;
  }
  *{box-sizing:border-box; margin:0; padding:0;}
  html,body{height:100%;}
  body{
    font-family:'Outfit', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    color:var(--ink); min-height:100vh;
    display:flex; align-items:center; justify-content:center; padding:24px 16px;
    background:
      radial-gradient(circle at 15% 10%, var(--bg-peach) 0%, transparent 45%),
      radial-gradient(circle at 90% 20%, var(--bg-violet) 0%, transparent 45%),
      radial-gradient(circle at 80% 95%, var(--bg-sky) 0%, transparent 50%),
      radial-gradient(circle at 10% 90%, var(--bg-mint) 0%, transparent 45%),
      #FFF8F0;
    position:relative; overflow:hidden;
  }
  .blob{position:fixed; border-radius:50%; filter:blur(70px); opacity:.35; pointer-events:none; transition:opacity .6s ease, background .6s ease;}
  .b1{width:320px; height:320px; background:var(--blob-violet); top:-90px; right:-80px;}
  .b2{width:280px; height:280px; background:var(--blob-teal); bottom:-90px; left:-70px;}
  .b3{width:240px; height:240px; background:var(--blob-orange); top:38%; left:-110px; opacity:.18;}
  body.on .b3{opacity:.5;}
  body.on .b2{background:var(--blob-pink);}
  .card{
    position:relative; z-index:1; width:100%; max-width:380px;
    background:var(--card); backdrop-filter:blur(18px); -webkit-backdrop-filter:blur(18px);
    border:1px solid var(--card-border); border-radius:28px; padding:28px 26px 22px;
    box-shadow:0 18px 50px rgba(90,60,150,.12); text-align:center;
  }
  header{display:flex; align-items:center; justify-content:space-between; margin-bottom:6px; text-align:left;}
  h1{font-size:20px; font-weight:700; letter-spacing:-.01em;}
  .sub{font-size:13px; color:var(--ink-soft); margin-top:2px;}
  .chip{display:inline-flex; align-items:center; gap:6px; font-size:12px; font-weight:600; padding:6px 11px; border-radius:999px; background:var(--good-bg); color:var(--good);}
  .chip::before{content:""; width:7px; height:7px; border-radius:50%; background:currentColor;}
  .chip.offline{background:var(--bad-bg); color:var(--bad);}
  .stage{position:relative; height:210px; display:flex; align-items:center; justify-content:center; margin:6px 0 4px;}
  .halo{position:absolute; width:230px; height:230px; border-radius:50%; background:radial-gradient(circle, rgba(255,181,71,.65) 0%, rgba(255,181,71,.22) 45%, transparent 70%); opacity:0; transform:scale(.7); transition:opacity .45s ease, transform .45s ease;}
  body.on .halo{opacity:1; transform:scale(1);}
  svg.bulb{position:relative; width:112px; height:160px; overflow:visible;}
  .glass{fill:#EEEAF6; stroke:#CFC8E0; stroke-width:2; transition:fill .35s ease, stroke .35s ease;}
  .filament{fill:none; stroke:#B9B2CC; stroke-width:3; stroke-linecap:round; stroke-linejoin:round; transition:stroke .35s ease;}
  .cap{fill:#CFC8E0;}
  body.on .glass{fill:#FFE29A; stroke:#FFB547;}
  body.on .filament{stroke:#FB923C;}
  .state{font-size:44px; font-weight:800; letter-spacing:-.03em; line-height:1;}
  .state-sub{font-size:14px; color:var(--ink-soft); margin-top:6px; min-height:20px;}
  .switch{
    appearance:none; -webkit-appearance:none; border:none; cursor:pointer; position:relative; display:block; margin:24px auto 6px;
    width:156px; height:76px; border-radius:999px; background:#D9D3E8; box-shadow:inset 0 2px 6px rgba(36,27,58,.14);
    transition:background .3s ease; -webkit-tap-highlight-color:transparent;
  }
  .switch::after{content:""; position:absolute; top:7px; left:7px; width:62px; height:62px; border-radius:50%; background:#fff; box-shadow:0 4px 12px rgba(36,27,58,.25); transition:transform .3s cubic-bezier(.3,1.3,.5,1);}
  body.on .switch{background:linear-gradient(135deg,#FBBF50,#FB923C);}
  body.on .switch::after{transform:translateX(80px);}
  .switch:focus-visible{outline:3px solid var(--blob-violet); outline-offset:4px;}
  .switch:active::after{width:70px;}
  body.on .switch:active::after{transform:translateX(72px);}
  .switch:disabled{opacity:.5; cursor:not-allowed;}
  footer{margin-top:22px; padding-top:16px; border-top:1px solid rgba(36,27,58,.08); display:flex; justify-content:space-between; font-size:12.5px; color:var(--ink-faint);}
  footer b{color:var(--ink-soft); font-weight:600;}
  @media (prefers-reduced-motion:reduce){ *{transition:none !important;} }
</style>
</head>
<body>
<div class="blob b1"></div><div class="blob b2"></div><div class="blob b3"></div>
<main class="card">
  <header>
    <div>
      <h1>Room light</h1>
      <p class="sub">Controlled by your ESP32</p>
    </div>
    <span class="chip" id="conn">Connected</span>
  </header>
  <div class="stage">
    <div class="halo"></div>
    <svg class="bulb" viewBox="0 0 112 160" aria-hidden="true">
      <path class="glass" d="M56 6C28 6 8 27 8 54c0 17 8 29 17 40 6 7 9 13 9 21v6h44v-6c0-8 3-14 9-21 9-11 17-23 17-40C104 27 84 6 56 6z"/>
      <path class="filament" d="M42 118V84l14 14 14-14v34"/>
      <rect class="cap" x="36" y="124" width="40" height="10" rx="3"/>
      <rect class="cap" x="40" y="136" width="32" height="10" rx="3"/>
      <path class="cap" d="M46 148h20l-3 8H49z"/>
    </svg>
  </div>
  <div class="state" id="state">Off</div>
  <p class="state-sub" id="stateSub">Tap the switch to turn it on</p>
  <button class="switch" id="sw" role="switch" aria-checked="false" aria-label="Room light"></button>
  <footer>
    <span>Pin <b>GPIO 2</b></span>
    <span id="ip"></span>
  </footer>
</main>
<script>
  const sw = document.getElementById('sw');
  const stateEl = document.getElementById('state');
  const subEl = document.getElementById('stateSub');
  const conn = document.getElementById('conn');
  document.getElementById('ip').textContent = location.hostname;
  let on = false;
  let busy = false;

  function render(){
    document.body.classList.toggle('on', on);
    sw.setAttribute('aria-checked', String(on));
    stateEl.textContent = on ? 'On' : 'Off';
    subEl.textContent = on ? 'The LED on GPIO 2 is lit' : 'Tap the switch to turn it on';
  }

  function setOnline(ok){
    conn.textContent = ok ? 'Connected' : 'Offline';
    conn.classList.toggle('offline', !ok);
    sw.disabled = !ok;
    if(!ok) subEl.textContent = 'Cannot reach the ESP32. Check it is powered and on WiFi.';
  }

  async function api(path){
    const res = await fetch(path, { cache: 'no-store' });
    if(!res.ok) throw new Error('bad response');
    return res.json();
  }

  async function load(){
    try{ on = (await api('/api/led')).on; setOnline(true); render(); }
    catch(e){ setOnline(false); }
  }

  sw.addEventListener('click', async () => {
    if(busy) return;
    busy = true;
    try{
      on = (await api('/api/led?state=' + (on ? 0 : 1))).on;
      setOnline(true); render();
    }catch(e){ setOnline(false); }
    busy = false;
  });

  load();
</script>
</body>
</html>
)rawliteral";

// ---------------------------------------------------------------------------
// Handlers
// ---------------------------------------------------------------------------
void handleRoot() {
  server.send(200, "text/html", PAGE);
}

// GET /api/led            -> returns the current state
// GET /api/led?state=1    -> turns the LED on, returns the new state
// GET /api/led?state=0    -> turns the LED off, returns the new state
void handleLed() {
  if (server.hasArg("state")) {
    ledOn = (server.arg("state") == "1");
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
    Serial.printf("LED %s\n", ledOn ? "ON" : "OFF");
  }
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", ledOn ? "{\"on\":true}" : "{\"on\":false}");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------
bool connectWiFi() {
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println("\nWiFi timed out. Check the name and password in secrets.h.");
      return false;
    }
    delay(400);
    Serial.print(".");
  }
  Serial.printf("\nConnected. Open http://%s in your browser.\n", WiFi.localIP().toString().c_str());
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!connectWiFi()) {
    // Wrong credentials or no signal: wait, then try again from the top.
    delay(5000);
    ESP.restart();
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/led", HTTP_GET, handleLed);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
