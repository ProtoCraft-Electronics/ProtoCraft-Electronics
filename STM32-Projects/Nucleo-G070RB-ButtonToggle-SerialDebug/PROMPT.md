# The AI Prompt Behind This Build

**ProtoCraft Electronics** — STM32 Bring-Up: CubeIDE Setup, First Blink, Button Toggle, and Serial Debug (NUCLEO-G070RB)

Board bring-up and pin selection happened in STM32CubeMX's GUI, not in a prompt. That part is point-and-click by design. What did get prompted was the logic that goes into the CubeMX-generated project: the button debounce, the toggle behavior, and the printf redirect. Shared as-is.

## The Prompt

> I have a CubeMX-generated STM32CubeIDE project for a NUCLEO-G070RB. USART2 is
> already configured at 115200 baud (this is the ST-Link virtual COM port).
> PA5 is configured as a GPIO output named LD4 (the onboard LED, active-high).
> PC13 is configured as a GPIO input named B1 (the onboard user button, idle
> HIGH, pressed pulls LOW).
>
> Write the code that goes in the USER CODE sections of main.c to:
> 1. Toggle LD4 every time B1 is pressed (one press = one state change, not
>    while held), using a non-blocking debounce based on HAL_GetTick(). No
>    HAL_Delay() anywhere in the main loop.
> 2. Redirect printf() to USART2 so debug text shows up in a serial
>    terminal, with a boot message on startup and a line every time the
>    button toggles the LED.

Same three things that made the ESP32 prompt work apply here. The exact hardware is named, down to which pins are already configured. The exact behavior is named: toggle on press, not on hold. The implementation is left open, how to structure the debounce, how to wire up the printf redirect, for the assistant to propose and the engineer to check.

## What Still Needed a Professional Eye

- **The debounce constant.** The first-pass answer used a 100 ms debounce window. Fine on paper, but sluggish against this specific button in testing. Brought down to 40 ms, the same value already proven on the ESP32 button project. No reason to run two different debounce times on the same channel for the same physical action.
- **The printf redirect target.** STM32CubeIDE's default `syscalls.c` already contains the loop that calls `__io_putchar()` for every character. It just doesn't define what that function does. The first answer overrode `_write()` from scratch instead, which works but duplicates a loop that already exists. Redirected to defining `__io_putchar()` instead, the one line ST's own template expects filled in.
- **Carriage returns.** The first draft used `\n` only. Most serial terminals reading raw UART want `\r\n`, or lines run into each other. Small thing, but it's the difference between a debug log that's readable and one that looks broken on the first try.
- **Idle state assumption for B1.** The prompt assumes B1 reads HIGH at idle, relying on an external pull-up already on the board. Worth confirming on your specific unit before trusting the logic. Some Nucleo revisions expect the internal pull-up enabled in CubeMX instead. Covered in the README.

## Try It Yourself

Get a CubeMX project generated with the same three peripherals configured, USART2 at 115200, LD4 as an output, B1 as an input, then run the prompt above yourself. Compare what you get against `main.c` in this repo, especially the printf redirect. If your assistant's first answer overrides `_write()` from scratch instead of just filling in `__io_putchar()`, you've caught the same review point caught here.

---

*Part of the ProtoCraft Electronics "STM32 Bring-Up" series. Full firmware and README: see this repository.*
