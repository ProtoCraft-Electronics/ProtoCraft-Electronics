# LED Gamma Correction — ESP32

**ProtoCraft Electronics** · [youtube.com/@ProtoCraftElectronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1)

> **Watch the full video:** [Why your LED fade looks broken — Gamma correction explained using ESP32](https://youtu.be/eCLPZqBViC4)

---

## What This Does

Runs two LEDs side by side to show the visible difference between linear PWM and gamma-corrected PWM.

| LED | Behaviour |
|-----|-----------|
| LED A — Linear | Raw PWM 0→255. Brightens too fast early, barely changes at the top. |
| LED B — Gamma  | Gamma-corrected PWM. Smooth, even brightness all the way through. |

---

## The Problem With Linear PWM

At PWM value `128` (50% duty cycle), the LED looks around **73% bright** — not 50%.

Human eyes perceive brightness on a logarithmic curve, not a straight line. Equal PWM steps do not look like equal brightness steps.

## The Fix

Apply a gamma correction formula before writing to the LED:

```
corrected = (input / 255) ^ 2.2 × 255
```

At input `128`:
- Linear sends PWM **128** → looks 73% bright
- Gamma sends PWM **56** → looks exactly 50% bright

---

## Hardware

```
ESP32 GPIO 25  →  220Ω  →  LED A (+)  →  GND    ← Linear
ESP32 GPIO 26  →  220Ω  →  LED B (+)  →  GND    ← Gamma
```

**Parts:**
- ESP32 Dev Kit
- 2× standard LEDs (same colour for fair comparison)
- 2× 220Ω resistors (any value 100Ω–470Ω works)
- Breadboard + jumper wires

---

## Requirements

| Tool | Version |
|------|---------|
| Arduino IDE | 2.x |
| ESP32 Arduino Core (Espressif) | **3.x** |

> **Important:** This sketch uses the new LEDC API from Core v3.x.
> `ledcAttach(pin, freq, resolution)` replaces the old `ledcSetup()` + `ledcAttachPin()`.
> Update via Arduino IDE → Boards Manager → esp32 by Espressif if needed.

---

## How to Run

1. Wire the circuit as shown above
2. Open `LED_Gamma.ino` in Arduino IDE
3. Select **ESP32 Dev Module** under Tools → Board
4. Select the correct COM port
5. Click Upload
6. Open **Serial Plotter** (Tools → Serial Plotter, 115200 baud) to see the gamma curve plotted live

---

## Key Values at a Glance

| Brightness input | Linear PWM sent | Gamma PWM sent |
|:---:|:---:|:---:|
| 64  | 64  | 13  |
| 128 | 128 | 56  |
| 192 | 192 | 118 |
| 255 | 255 | 255 |

---

## Folder Structure

```
LED_Gamma/
├── LED_Gamma.ino    ← Arduino sketch
└── README.md        ← This file
```

---

## Licence

MIT — free to use, modify, and share.

---

*Part of the ProtoCraft Electronics project series — real-world hardware engineering made accessible.*
