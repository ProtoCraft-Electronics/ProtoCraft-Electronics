# ESP-NOW Unicast — Talk to Exactly One Board (Part 2)

Three boards, each with two buttons and two LEDs. Every board can independently
address either of the other two by MAC — proving unicast lets you choose the
recipient, unlike broadcast (Part 1), which reaches everyone listening.

Button 1 unicasts to Peer 0. Button 2 unicasts to Peer 1. On receive, each
board checks *who* sent the packet (`src_addr`) and lights the matching LED —
so only the intended board reacts, and the other one stays dark.

## 🔌 Wiring / Parts List

**Classic ESP32 Dev Kit**

| Role | GPIO |
|---|---|
| Button → Peer 0 | GPIO 4 |
| Button → Peer 1 | GPIO 13 |
| LED ← Peer 0 | GPIO 18 |
| LED ← Peer 1 | GPIO 19 |

**ESP32-C3 Super Mini**

| Role | GPIO |
|---|---|
| Button → Peer 0 | GPIO 4 |
| Button → Peer 1 | GPIO 5 |
| LED ← Peer 0 | GPIO 6 |
| LED ← Peer 1 | GPIO 7 |

(C3 pin choice avoids strapping pins 2/8/9 and the USB-JTAG pins 18/19.)

**Boards used:** 3x ESP32 (mix of classic Dev Kit and C3 Super Mini).

**Note:** Breadboard-only prototype for the video — no custom PCB or schematic,
so there is no `hardware/` folder for this project.

## 🚀 Setup

1. Flash `firmware/get_mac_address/get_mac_address.ino` to each of the 3
   boards, one at a time. Note down each board's printed MAC address.
2. Open `firmware/unicast_mesh_node/unicast_mesh_node.ino` and, **for each
   board**, edit the top config block:
   - Paste in the MAC addresses of the *other two* boards as `peerMac0` /
     `peerMac1`.
   - Uncomment the pin block matching that board's hardware (classic Dev Kit
     or C3 Super Mini).
3. Flash the edited sketch to that board. Repeat for all 3 — the code
   structure is identical each time, only the config block changes.
4. Wire per the tables above and power all three boards.
5. Press a button — only the board matching that peer's MAC reacts. The third
   board (and the other peer's LED) stays dark.

## 🛠️ Requirements

- ESP32 Arduino Core v3.x
- No external libraries beyond `esp_now.h` and `WiFi.h` (bundled with the core)

## 📺 Video

Part 2 of the ESP-NOW mini-series on the
[ProtoCraft Electronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1) YouTube channel.
Watch: https://youtu.be/o7l_a4yrzts
Part 1 (broadcast) is at `ESP32-Projects/ESP-NOW-Example/`.
