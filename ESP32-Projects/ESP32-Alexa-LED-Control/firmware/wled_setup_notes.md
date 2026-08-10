# WLED — Alexa Setup (No Code)

WLED already ships with built-in Alexa/Hue-bridge emulation (via Espalexa under the hood). No sketch to write.

## Steps

1. Flash WLED to the ESP32 as normal (via wled.me web installer, or your existing WLED build)
2. In the LED preferences, set:
   - Data pin: GPIO5 (or wherever you wired it)
   - LED count: 30 (adjust to your strip)
3. Go to **Config → Sync Interfaces → Alexa Voice Control**
4. Enable Alexa Voice Control
5. Give the device a spoken-friendly name (e.g. "desk light") in Config → WiFi, under mDNS/device name — this is what Alexa will discover it as
6. Say "Alexa, discover devices"
7. Test: "Alexa, turn on desk light", "Alexa, set desk light to 50%", "Alexa, set desk light to blue"

## Recording notes
- Screen-record the Sync Interfaces toggle directly from the WLED web UI (works in any browser, no app needed)
- Phone screen-record the Alexa app's discovery process separately — cut together in post

## Known caveat (mention on camera)
Same underlying UPnP/SSDP Hue emulation as fauxmoESP — newer Echo Gen 3/4/Show hardware occasionally fails to discover it. If discovery doesn't work first try, reboot the ESP32 and re-run discovery.

## Don't confuse with WLED's other "Hue" feature
Sync Settings also has a separate "Philips Hue" section that syncs WLED's color *from* a real physical Hue bulb — that's a one-way color mirror, unrelated to Alexa control. Don't toggle that one for this demo.
