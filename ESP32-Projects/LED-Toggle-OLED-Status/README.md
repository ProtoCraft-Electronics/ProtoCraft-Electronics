# ESP32 LED Toggle with OLED Status Display

Toggle the onboard LED on an ESP32 Dev Kit with a push button on GPIO 4, with
a 1.3" I2C OLED that shows a boot splash on power-up and then a live LED
status screen. Built as part of the **ProtoCraft Electronics** "Protocol
Decoded" / ESP32 Foundation series.

---

## Features

- One-button toggle: press once for ON, press again for OFF — not
  press-and-hold.
- Non-blocking `millis()`-based debounce — no `delay()` calls in `loop()`,
  so the board stays free to take on more work later (sensors, Wi-Fi, etc.).
- OLED boot screen with the channel logo, followed automatically by a live
  LED status screen that updates the instant the button is pressed.
- Self-contained sketch: firmware + logo bitmap only, no external assets
  needed to build.

## Hardware Required

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32 Dev Kit | Any variant (DOIT ESP32 DEVKIT V1 or similar). Confirm your onboard LED pin — see [Onboard LED caveat](#onboard-led-caveat) below. |
| 1 | 1.3" OLED display | SSD1306 driver, I2C interface, 128x64. If your module uses an SH1106 driver instead, see [Display driver mismatch](#display-driver-mismatch). |
| 1 | Momentary push button | Any standard 2-pin or 4-pin tactile switch |
| — | Jumper wires | For I2C (SDA/SCL/VCC/GND) and the button |
| — | Breadboard | Optional, for prototyping before a permanent build |

No external resistor is needed for the button — the sketch uses the ESP32's
internal pull-up (`INPUT_PULLUP`).

## Wiring

| ESP32 Pin | Connects to | Signal |
|---|---|---|
| 3V3 | OLED VCC | Power |
| GND | OLED GND | Ground |
| GPIO 21 | OLED SDA | I2C data (default ESP32 I2C pin) |
| GPIO 22 | OLED SCL | I2C clock (default ESP32 I2C pin) |
| GPIO 4 | Button, one leg | Digital input, `INPUT_PULLUP` |
| GND | Button, other leg | Ground |

The onboard LED needs no external wiring — it's driven directly by the
firmware via `LED_BUILTIN`.

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software) 2.x
- ESP32 board support package — install via **File → Preferences → Additional
  Boards Manager URLs**:
  `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
  then install **esp32** from **Tools → Board → Boards Manager**.
- Libraries (install via **Sketch → Include Library → Manage Libraries**):
  - `Adafruit GFX Library`
  - `Adafruit SSD1306`

## Getting Started

1. Wire the hardware as described above.
2. Clone this repository (or download this project's folder) and open
   `firmware/protocraft_led_toggle/protocraft_led_toggle.ino` in the Arduino
   IDE. Keep `icon_bitmap.h` in the same folder — it's a second tab the IDE
   loads automatically.
3. Select **Tools → Board** → your ESP32 Dev Kit variant, and the correct
   **Port**.
4. Click **Upload**.
5. On boot, the OLED shows the ProtoCraft Electronics logo for ~3 seconds,
   then switches to the LED status screen. Press the button to toggle the
   LED and watch the screen update.

## How It Works

- **Debounce**: the button is read every loop iteration; `handleButton()`
  tracks the last raw reading and only accepts a new state once it's been
  stable for `DEBOUNCE_DELAY` (40 ms), using `millis()` rather than
  `delay()` so nothing else in `loop()` gets blocked.
- **Toggle logic**: the LED state only flips on the falling edge (button
  just pressed), so holding the button down doesn't repeatedly toggle it.
- **Boot screen**: `showBootScreen()` draws the 32x32 monochrome chip-icon
  bitmap from `icon_bitmap.h` plus the wordmark, held on screen for
  `BOOT_SPLASH_MS` (3000 ms) before the first status draw.
- **Status screen**: `drawLedStatus()` redraws the full screen each time the
  LED state changes — simple and reliable for a screen that updates
  infrequently (on button press only).

## Customization

| Want to... | Change this |
|---|---|
| Use a different LED pin | Set `LED_PIN` to the correct GPIO (see caveat below) |
| Use a different button pin | Set `BUTTON_PIN` |
| Adjust debounce sensitivity | Change `DEBOUNCE_DELAY` (ms) |
| Show the boot screen longer/shorter | Change `BOOT_SPLASH_MS` (ms) |
| Use your own logo | Regenerate `icon_bitmap.h` from a PNG via [image2cpp](https://javl.github.io/image2cpp/) (output format: "Adafruit GFX bitmap"), keep it within 128x64 |
| Use an SH1106 display instead | See below |

### Onboard LED Caveat

Not every ESP32 Dev Kit board exposes a usable onboard LED via
`LED_BUILTIN` — some clones have no onboard user LED at all, others wire it
to a non-obvious GPIO (GPIO 2 is common on "DOIT ESP32 DEVKIT V1" style
boards). If nothing lights up, hard-code `LED_PIN` to the GPIO your specific
board actually uses.

### Display Driver Mismatch

1.3" OLED modules are sold under two visually identical but
code-incompatible drivers: **SSD1306** and **SH1106**. This sketch assumes
SSD1306. If the display stays blank:

- Try I2C address `0x3D` instead of `0x3C` (`OLED_I2C_ADDR` in the sketch).
- If it's actually an SH1106 panel, swap the `Adafruit_SSD1306` library and
  calls for `Adafruit_SH110X` (`Adafruit_SH1106G`) — the API is nearly
  identical.

## Repository Structure

```
LED-Toggle-OLED-Status/
├── firmware/
│   └── protocraft_led_toggle/
│       ├── protocraft_led_toggle.ino   # Main firmware
│       └── icon_bitmap.h                # Channel logo, pre-converted to a 1-bit bitmap
├── PROMPT.md                            # The AI prompt used to build this project
└── README.md                            # This file
```

## Video

[ProtoCraft Electronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1) YouTube channel.
Watch: (link TBD)
