# STM32 Bring-Up: CubeIDE Setup, First Blink, Button Toggle, Serial Debug

First real firmware on a NUCLEO-G070RB using STM32CubeIDE and the HAL, not the Arduino wrapper. Covers project setup, the clock tree, a non-blocking button-driven LED toggle, and getting `printf()` visible in a serial terminal. Part of **ProtoCraft Electronics**'s "STM32 Bring-Up" series.

## What This Builds

- LD4 (the onboard LED) toggles once per B1 button press, using the same one-press-one-state-change behavior as the ESP32 LED toggle project on this channel.
- Debug text over the ST-Link's virtual COM port: a boot message on startup, and a line every time the LED state changes.
- Everything timed with `HAL_GetTick()`. No `HAL_Delay()` in the main loop.

## Hardware Required

| Qty | Part | Notes |
|---|---|---|
| 1 | NUCLEO-G070RB | Or any STM32G0 Nucleo-64 board. Pin names may differ slightly, check your board's user manual |
| 1 | Micro-USB cable | Connects to the ST-Link side of the board. This is both power and the debug/serial link, no external USB-to-serial adapter needed |

No breadboard, no external LED, no external button. Everything used in this build is already on the board.

## Software Requirements

- STM32CubeIDE (free from ST, includes CubeMX, no separate downloads needed for the IDE, the compiler, or the pin/clock configurator)
- ST-Link USB driver, Windows only (see the step-by-step below)
- A serial terminal to read the debug output. The Arduino IDE's Serial Monitor works fine if you already have it installed, or use a dedicated terminal (PuTTY on Windows, CoolTerm on macOS, `screen`/`minicom` on Linux)

### Installing STM32CubeIDE (Windows)

1. Go to [st.com/en/development-tools/stm32cubeide.html](https://www.st.com/en/development-tools/stm32cubeide.html) and click the download button. ST gates the download behind a short form, either a free myST account or just an email, plus a license agreement (SLA0048-ish, standard ST terms) to accept.
2. Pick the Windows installer, it downloads as something like `st-stm32cubeide_<version>_Win64.exe`.
3. Right-click it, **Run as administrator** (required, the installer won't proceed otherwise), accept the license, and keep the default install location unless you have a reason not to.
4. That's the whole install. STM32CubeMX and the GCC ARM toolchain are bundled in, no separate downloads, no separate Java install either, CubeIDE brings its own.

### Installing the ST-Link USB driver (Windows, mandatory)

1. Go to [st.com/en/development-tools/stsw-link009.html](https://www.st.com/en/development-tools/stsw-link009.html) and download STSW-LINK009 (same email-gated flow as above).
2. Unzip it, then run the 64-bit installer inside (`dpinst_amd64.exe`) as administrator.
3. Do this **before** plugging the Nucleo board in for the first time, Windows won't enumerate it correctly otherwise.

If you've already used this board with the Arduino STM32 core (STM32duino) via the "STM32CubeProgrammer (SWD)" upload method, this driver is likely already installed, that upload path depends on it too. If you used the "Mass Storage" upload method instead, it doesn't, and this will be new.

**Verify the board is detected**: plug it in, open Windows Device Manager, and check under **Universal Serial Bus devices**. You should see "STMicroelectronics STLink dongle" listed cleanly, not under "Other devices" with a warning icon. If it's under "Other devices," right-click it, Update driver, and point it at the unzipped STSW-LINK009 folder.

### macOS and Linux

Same download/account flow, but you get a `.dmg`/`.pkg` (macOS) or a `.sh`/package (Linux) instead of an `.exe`. macOS may ask you to approve the installer once in System Settings → Privacy & Security. Neither platform needs the ST-Link driver step above, both usually enumerate the board without one.

### Optional, not required for this build

STM32CubeProgrammer, a standalone flashing/memory-viewing utility, useful for other tasks like erasing flash or updating the ST-Link's own firmware. CubeIDE's Run/Debug button flashes the board on its own, so this isn't needed just to build and run this project.

## CubeMX Project Setup

1. New STM32 Project, **Board Selector** tab (not MCU Selector), search `NUCLEO-G070RB`, select it.
2. When prompted "Initialize all peripherals with their default Mode?", choose **Yes**. This auto-configures three things you'd otherwise wire up by hand:
   - `LD4`, PA5, GPIO output (the onboard LED)
   - `B1`, PC13, GPIO input (the onboard user button)
   - `USART2`, PA2/PA3, 115200 baud, already wired through the ST-Link to your USB port as a virtual COM port
3. Leave the **Clock Configuration** tab untouched for this build. The G0's default clock tree runs SYSCLK directly off the internal 16 MHz HSI oscillator, no PLL, no external crystal, and that's enough for a blink and a 115200 baud UART. Worth opening the tab once just to see it before moving on.
4. Generate the code. This creates the full CubeIDE project with `main.c` already containing the peripheral init calls and the `USER CODE BEGIN/END` markers.

## Getting Started

1. Complete the CubeMX setup above and generate the project.
2. Open the generated `main.c` and add the code from this repo's `main.c` into the matching `USER CODE` sections: includes, private variables, the function prototype, inside `while(1)`, and the new `HandleButton()` function.
3. Build and flash using the run/debug button in CubeIDE. The ST-Link on the Nucleo board handles programming, no separate programmer needed.
4. Open your serial terminal on the ST-Link's COM port at 115200 baud, 8N1.
5. You should see a boot message immediately, then a new line every time you press B1, alongside LD4 toggling on and off.

## How It Works

- **Debounce**: `HandleButton()` reads B1 every loop iteration and only accepts a new state once it's held stable for `BUTTON_DEBOUNCE_MS` (40 ms), tracked with `HAL_GetTick()` instead of blocking with `HAL_Delay()`. Same non-blocking pattern used across this channel's ESP32 projects, just with the STM32 HAL's tick function instead of `millis()`.
- **Toggle logic**: the LED only flips on the falling edge of B1 (button just pressed), so holding it down doesn't repeatedly toggle the LED.
- **printf redirect**: STM32CubeIDE's default `syscalls.c` already loops over every character passed to `printf()` and calls `__io_putchar()` for each one. It just doesn't define that function. This repo's `main.c` defines it to call `HAL_UART_Transmit()` on USART2, which gets the text out over the ST-Link's virtual COM port.

## Customization

| Want to... | Change this |
|---|---|
| Use a different debounce window | Change `BUTTON_DEBOUNCE_MS` |
| Print floats (sensor readings, etc.) with `printf` | Add `-u _printf_float` to the linker flags. The default newlib-nano build here only supports strings and integers |
| Move to a different STM32G0 Nucleo board | Re-check the LED and button pin names in CubeMX. `LD4`/PA5 and `B1`/PC13 are confirmed for the G070RB specifically, other boards in the family may differ |

### Idle state caveat for B1

This build assumes B1 reads HIGH at idle and LOW when pressed, relying on an external pull-up already present on the board. If your board reads LOW at idle with nothing pressed, enable the internal pull-up on PC13 in CubeMX (GPIO settings, Pull-up) and regenerate.

## Repository Structure

```
Nucleo-G070RB-ButtonToggle-SerialDebug/
├── firmware/
│   └── button_toggle_serial_debug/
│       └── main.c      # USER CODE sections only, merge into your CubeMX-generated main.c
├── PROMPT.md           # The AI prompt used for the logic layer
└── README.md           # This file
```

## License

Released under the MIT License, see `LICENSE`. Use it, modify it, ship it in your own projects.

## Credits

Built by **ProtoCraft Electronics**, hardware tutorials from a working hardware design engineer, no fluff, real debugging included.
More builds: `github.com/ProtoCraft-Electronics/ProtoCraft-Electronics`
