# ESP-NOW Example — One Switch, Two Boards, Zero Coordination

One transmitter (switch) broadcasts a single ESP-NOW packet. Two independent
receivers — each running the identical receiver sketch, with no knowledge of
each other — react at the same instant. No WiFi network, no pairing, no router.

## 🔌 Wiring / Parts List

| Board | Component | GPIO | Notes |
|---|---|---|---|
| Transmitter | Switch | GPIO 4 | `INPUT_PULLUP` — other leg of switch to GND, no external resistor needed |
| Receiver 1 | LED | GPIO 2 | LED + ~220Ω resistor to GND |
| Receiver 2 | LED | GPIO 2 | Identical wiring to Receiver 1 — same sketch flashed to both boards |

No MAC address hardcoding required — the sender uses the ESP-NOW broadcast
address (`FF:FF:FF:FF:FF:FF`), so both receivers pick up the same packet
independently.

**Boards used:** 3x ESP32 Dev Kit (or ESP32-C3 Super Mini) — 1 transmitter, 2 receivers.

**Note:** This is a breadboard-only prototype for the video — no custom PCB or
schematic, so there is no `hardware/` folder for this project.

## 🚀 Setup

1. Flash `firmware/espnow_sender/espnow_sender.ino` to the transmitter board.
2. Flash `firmware/espnow_receiver/espnow_receiver.ino` to **both** receiver
   boards — the sketch is identical for each, no code changes needed.
3. Wire per the table above and power all three boards.
4. Flip the switch — both LEDs react simultaneously.

## 🛠️ Requirements

- ESP32 Arduino Core v3.x
- No external libraries beyond `esp_now.h` and `WiFi.h` (bundled with the core)

## 📺 Video

Part of the [ProtoCraft Electronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1) YouTube channel.