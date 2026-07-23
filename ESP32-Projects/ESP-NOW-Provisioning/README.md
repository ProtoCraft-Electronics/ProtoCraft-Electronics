# ESP-NOW SoftAP Provisioning — No Reflashing to Reconfigure (Part 3)

Same 3-board unicast mesh as Part 2, but peer MAC addresses are no longer
hardcoded in the sketch. Hold a button at power-on, and the board serves a
web config page over its own WiFi access point — enter the two peer MACs,
hit save, and it reboots straight into normal operation with those peers.

## How it works

- **Hold `BUTTON_PEER0` while powering on** → board enters **config mode**:
  starts a SoftAP (`ProtoCraft-Config-XXXX`), serves a page at
  `192.168.4.1` showing this board's own MAC and two input fields for the
  peer MACs, writes them to flash (NVS via `Preferences`) on save, then
  reboots.
- **Normal power-on** (button not held) → loads the two peer MACs from
  flash and runs the same ESP-NOW unicast logic as Part 2: button 1 sends
  to Peer 0, button 2 sends to Peer 1, and the receive callback checks
  `src_addr` to light the matching LED.
- If no peers have been configured yet, both LEDs slow-blink as an
  "unconfigured" indicator instead of silently doing nothing.

## 🔌 Wiring / Parts List

Identical to Part 2 — this is a firmware change, not a wiring one.

**Classic ESP32 Dev Kit**

| Role | GPIO |
|---|---|
| Button → Peer 0 (also the config-mode entry button) | GPIO 4 |
| Button → Peer 1 | GPIO 13 |
| LED ← Peer 0 | GPIO 18 |
| LED ← Peer 1 | GPIO 19 |

**ESP32-C3 Super Mini**

| Role | GPIO |
|---|---|
| Button → Peer 0 (also the config-mode entry button) | GPIO 4 |
| Button → Peer 1 | GPIO 5 |
| LED ← Peer 0 | GPIO 6 |
| LED ← Peer 1 | GPIO 7 |

**Boards used:** 3x ESP32 (mix of classic Dev Kit and C3 Super Mini).

**Note:** Breadboard-only prototype — no custom PCB or schematic, so there
is no `hardware/` folder for this project.

## 🚀 Setup

1. Flash `firmware/espnow_softap_config/espnow_softap_config.ino` to all
   3 boards — identical code on every board, nothing to edit per board
   except the pin block at the top (classic Dev Kit vs. C3 Super Mini).
2. Power on a board while holding `BUTTON_PEER0` to enter config mode.
3. Connect your phone/laptop to the WiFi network it creates
   (`ProtoCraft-Config-XXXX`), then browse to `http://192.168.4.1`.
4. The page shows this board's own MAC. Enter the MAC addresses of the
   *other two* boards (get these by putting each of them into config mode
   in turn and reading the MAC off their own page) as Peer 0 / Peer 1,
   then hit **Save & Reboot**.
5. Repeat for the remaining boards, then wire per the tables above and
   power all three. Buttons and LEDs behave exactly as in Part 2.

## ⚠️ Security note

Config mode's access point has no password, and the page is plain HTTP.
That's a deliberate simplification for a bench demo — not something to
ship in an actual product. A real device would want at minimum a WPA2 AP
password, ideally HTTPS or a BLE-based config flow instead of open HTTP.

## 🛠️ Requirements

- ESP32 Arduino Core v3.x
- Libraries used: `esp_now.h`, `WiFi.h`, `WebServer.h`, `Preferences.h`
  (all bundled with the core — no external library installs needed)

## 📺 Video

Part 3 of the ESP-NOW mini-series on the
[ProtoCraft Electronics](https://www.youtube.com/@ProtoCraftElectronics) YouTube channel.
Part 1 (broadcast): `ESP32-Projects/ESP-NOW-Example/`
Part 2 (unicast): `ESP32-Projects/ESP-NOW-Unicast/`
