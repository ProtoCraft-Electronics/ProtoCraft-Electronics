# ProtoCraft Electronics — Project Repository

Code, schematics, PCB designs, and hardware resources for every video on the
[ProtoCraft Electronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1) YouTube channel.
Built by a professional hardware design engineer with 13 years of real-world experience
across digital, analog, high-speed, and power supply domains.

## 📁 Repository Structure

```
ProtoCraft-Electronics/
│
├── ESP32-Projects/               ← YouTube/IoT projects with firmware + hardware
│   ├── ESP32-Weather-Station/
│   │   ├── firmware/             ← Arduino / ESP-IDF code
│   │   ├── hardware/             ← Schematics, PCB, BOM
│   │   └── README.md
│   └── ESP-NOW-Example/
│       ├── firmware/
│       ├── hardware/
│       └── README.md
│
├── Power-Supply-Designs/         ← Pure hardware designs, no firmware
│   ├── TPS543021-Buck-Converter/
│   │   ├── schematic.pdf
│   │   ├── calculations.xlsx
│   │   ├── BOM.xlsx
│   │   └── README.md
│   └── LCD-Backlight-TPS92200/
│       ├── schematic.pdf
│       └── BOM.xlsx
│
├── Protection-Circuits/          ← ESD, overvoltage, reverse polarity etc
│   └── I2C-ESD-Protection/
│       ├── schematic.pdf
│       └── README.md
│
└── Reference-Designs/            ← General hardware references and templates
```

## 🚀 Getting Started

Each project folder contains:
- `README.md` — overview, wiring diagram, parts list, and setup instructions
- `firmware/` — Arduino / ESP-IDF sketch (.ino, .c, .h)
- `hardware/` — KiCad files, schematic PDF, PCB layout, BOM
- `docs/` — wiring diagrams, images, datasheets (where applicable)

## 🛠️ Hardware & Tools Used

- **Microcontrollers:** ESP32 (various boards)
- **Firmware IDE:** Arduino IDE 2.x / ESP-IDF with VS Code
- **PCB Design:** KiCad 8
- **Power Design:** TI designs (TPS series), Analog Devices
- **Key Libraries:** FastLED, Adafruit SSD1306, Adafruit GFX
- **Test Equipment:** Oscilloscope, Multimeter, Logic Analyser

## 📺 YouTube Channel

[youtube.com/@ProtoCraftElectronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1)

New videos covering ESP32 projects, PCB design, power supply design,
and real-world professional hardware engineering.

## 👨‍💻 About

This repository is maintained by a hardware design engineer with 13 years of
experience in digital, analog, high-speed, and power supply domains.
All designs are based on real-world engineering practice — not just hobbyist tutorials.

## 📄 License

Code in this repository is released under the [MIT License](LICENSE).
Schematics and hardware designs are released under
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
