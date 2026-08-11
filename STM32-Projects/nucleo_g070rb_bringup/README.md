# Nucleo G070RB Bring-Up

Getting a NUCLEO-G070RB running in Arduino IDE, from a blank install to a working button/LED demo. Companion code for the ProtoCraft Electronics STM32/Nucleo mini-series.

## Hardware

- NUCLEO-G070RB (STM32G070RB, Cortex-M0+, 128 KB flash, 36 KB RAM)
- No hardware FPU. No native USB peripheral, onboard ST-LINK/V2-1 handles both programming and the virtual COM port.

## Setup

1. In Arduino IDE, add this URL under Preferences > Additional Boards Manager URLs:
   `https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json`
2. Tools > Board > Boards Manager, search "STM32 MCU based boards" (Contributed, by STMicroelectronics), install.
3. Connect the board via its ST-LINK USB port (CN1). No external programmer required.
4. Tools > Board > STM32 boards groups > Nucleo-64 > Nucleo G070RB.
5. Tools > Upload method > STM32CubeProgrammer (SWD).
6. Install STM32CubeProgrammer if the IDE prompts for it.
7. Upload `firmware/nucleo_g070rb_bringup/nucleo_g070rb_bringup.ino`.

## What the demo does

Reads the onboard user button (B1) and toggles the onboard LED (LD4) on each press, logging state changes to the Serial Monitor at 115200 baud over the ST-LINK virtual COM port.

## Notes

- "Arduino compatible" refers to the Arduino Uno-shaped header, not the toolchain. This board runs on STM32duino, a community core that ST contributes to but does not officially maintain as part of the Arduino ecosystem.
- If porting code from an F4 or F7 board, check for hardware floating point assumptions first. This part doesn't have an FPU.

---

MIT License (code). Part of the [ProtoCraft Electronics](https://github.com/ProtoCraft-Electronics/ProtoCraft-Electronics) organization repo.
