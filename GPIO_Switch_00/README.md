# GPIO_Switch_00 — ESP32 Internal Pull-up Example

Part of the [ProtoCraft Electronics](https://www.youtube.com/@ProtocraftElectronics) tutorial series.

## What it does

Reads a push button on an ESP32 GPIO pin and lights an LED while the button is pressed, using the internal pull-up resistor (no external resistor needed).

- **Button pin:** GPIO 4
- **LED pin:** GPIO 2
- Button pressed → pin reads `LOW` → LED turns on, "Button Pressed" printed to Serial
- Button released → pin reads `HIGH` → LED turns off, "Button Released" printed to Serial
- Serial output at 115200 baud

## Wiring

- Button: one leg to GPIO 4, the other leg to GND
- LED: anode to GPIO 2 (through a current-limiting resistor), cathode to GND

See the included reference images (`ESP32_IO_SW_01.png`, etc.) for the circuit layout.

## Files

- `GPIO_Switch_00.ino` — Arduino sketch for the ESP32
- `*.png` — circuit/reference images
