# ESP32 + Alexa LED Strip Control — 3 Methods Compared

Companion code for the ProtoCraft Electronics video comparing three ways to control a WS2812B LED strip with Alexa, using an ESP32.

## Methods covered

1. **WLED** — built-in Alexa/Hue emulation, zero custom code
2. **fauxmoESP** — local Hue/WeMo emulation library, on/off + brightness only
3. **SinricPro** — cloud-based official Alexa Smart Home integration, full color + brightness

## Hardware (breadboard only, no custom PCB)

- ESP32 Dev Kit (any standard WROOM32 board)
- WS2812B LED ring, 32 LEDs — adjust `NUM_LEDS` if yours differs
- 5V power supply sized for your strip/ring length (32 LEDs ≈ 1.9A at full white/full brightness — a standard 2A 5V USB supply is enough for this demo, but power it from the external supply, not the ESP32's onboard regulator)
- Data line: GPIO5 (change `LED_PIN` if you wire differently)
- Common ground between ESP32 and strip power supply
- Recommended: 300–500Ω resistor in series on the data line, and a large capacitor (1000µF) across the strip's 5V/GND — standard WS2812B practice, keeps signal clean on camera

## Folder structure

```
ESP32-Alexa-LED-Control/
├── README.md
├── script.md              (dual-column voiceover + shot script, timestamped)
├── metadata.md             (titles, description, tags)
├── thumbnail_brief.md
├── shot_list.md            (single-session shot list)
├── shorts_plan.md          (3 Shorts repurposed from this shoot)
└── firmware/
    ├── wled_setup_notes.md          (no code — just the settings screens to record)
    ├── fauxmoESP_LED_Alexa/
    │   ├── fauxmoESP_LED_Alexa.ino
    │   └── secrets.h.example        (copy to secrets.h, fill in Wi-Fi creds — gitignored)
    └── SinricPro_LED_Alexa/
        ├── SinricPro_LED_Alexa.ino
        └── secrets.h.example        (copy to secrets.h, fill in Wi-Fi + SinricPro creds — gitignored)
```

## Quick comparison

| | WLED | fauxmoESP | SinricPro |
|---|---|---|---|
| Custom code needed | No | Yes | Yes |
| Cost | Free | Free | Free (≤3 devices), then $3/device/yr or $39.99/yr unlimited |
| Local or cloud | Local | Local | Cloud |
| Color support | Yes | No (brightness only) | Yes |
| Reliability on newer Echo (Gen 3/4/Show) | Same caveat as fauxmoESP | Least reliable — unofficial UDP/SSDP emulation | Most reliable — official cloud-to-cloud integration |
| Best for | Already running WLED, want zero-effort voice control | Learning how device emulation works, fully local/no-account | Anything you actually want to keep working |

Full reasoning and on-camera recommendation is in `script.md`, Section 7 (Comparison).
