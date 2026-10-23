# STM32 Clock Speed Prime Benchmark: Change the Clock Tree, Measure the Result

Change the clock configuration of an STM32G0 and measure what it actually does to performance. A prime-number benchmark runs for a fixed time window at 8, 16, 32, 48 and 64 MHz, prints the result over the serial port, and ends with a summary table. Companion project to the [ProtoCraft Electronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1) YouTube channel.

Tested on a NUCLEO-G070RB with STM32CubeIDE, Debug build (-O0).

## What This Builds

- A benchmark that reconfigures the clock tree at runtime (HSI divider, PLL, flash wait states), counts primes by trial division for a fixed window, and reports the result for each clock.
- Serial output over the ST-LINK virtual COM port: one line per run, then a summary table with divisions per second and speed relative to the first run.
- LD4 (the onboard LED) is on while a run is in progress, so a phone stopwatch can confirm the window length.
- A second mode (`SWEEP_MODE 0`) that runs the same benchmark once at whatever clock STM32CubeMX generated. Use it to compare the clock settings you pick in the CubeMX GUI against the runtime sweep.

The point is to measure instead of assume. A clock that is 8 times faster does not do 8 times the work, and this project shows how far off it is and gives you a way to look for the reasons.

## Hardware Required

| Qty | Part | Notes |
|---|---|---|
| 1 | NUCLEO-G070RB | STM32G070RB, Cortex-M0+, 64 MHz max, no FPU, no hardware divider |
| 1 | Micro-USB cable | Connects to the ST-LINK side of the board. Power, programming and the serial port all go through it |

Nothing else is needed. The LED (LD4), the button (B1) and the serial port (USART2 on the ST-LINK) are all on the board.

Optional: a multimeter on the IDD jumper if you want to look at current at different clocks. A scope is not needed.

## Software Requirements

- STM32CubeIDE (includes CubeMX, the compiler and the debugger)
- ST-LINK USB driver (Windows only, see the ButtonToggle project below for the install steps)
- A serial terminal at 115200 baud, 8N1 (PuTTY on Windows, CoolTerm on macOS, `screen` or `minicom` on Linux, or the Arduino IDE Serial Monitor)

If you have not set up CubeIDE or the serial terminal before, the [Nucleo-G070RB-ButtonToggle-SerialDebug](../Nucleo-G070RB-ButtonToggle-SerialDebug/README.md) project in this repo walks through the installation step by step.

## CubeMX Project Setup

1. New STM32 Project, **Board Selector** tab (not MCU Selector), search `NUCLEO-G070RB`, select it.
2. When asked "Initialize all peripherals with their default Mode?", choose **Yes**. This gives you:
   - `LD4` on PA5, GPIO output (the onboard LED)
   - `B1` on PC13, GPIO input (the onboard user button)
   - `USART2` on PA2/PA3 at 115200 baud, connected through the ST-LINK as a virtual COM port
3. Leave the Clock Configuration tab alone for the first run. The default clock tree runs from the internal 16 MHz HSI oscillator.
4. Generate the code.

The firmware relies on these generated names: `huart2`, `LD4_Pin`, `LD4_GPIO_Port`, `B1_Pin`, `B1_GPIO_Port`. If your project uses different names, change them in `benchmark.c`.

## Getting Started

1. Copy `firmware/clock_speed_prime_benchmark/benchmark.h` into your project's `Core/Inc/`.
2. Copy `firmware/clock_speed_prime_benchmark/benchmark.c` into your project's `Core/Src/`.
3. In `Core/Src/main.c`, add the include inside the Includes block:

```c
/* USER CODE BEGIN Includes */
#include "benchmark.h"
/* USER CODE END Includes */
```

4. Still in `main.c`, call the benchmark after the peripherals are initialised:

```c
/* USER CODE BEGIN 2 */
Benchmark_Run();
/* USER CODE END 2 */
```

`Benchmark_Run()` never returns, so nothing after it runs.

5. Build and flash with the run/debug button. The ST-LINK programs the board.
6. Open the serial terminal on the ST-LINK COM port at 115200 baud, 8N1.
7. Press B1. LD4 turns on for each run. With the default 10 s window, the full sweep takes a little under a minute (five runs). With 30 s it takes about two and a half minutes.

Printing uses `HAL_UART_Transmit()` directly, so you do not need to retarget `printf()` or add `__io_putchar()`.

## Switches in benchmark.c

Edit the `#define` lines near the top of `benchmark.c`.

| Define | Default | What it does |
|---|---|---|
| `SWEEP_MODE` | 1 | 1 runs all five presets one after another. 0 runs once at whatever clock CubeMX configured, for the GUI-driven comparison. |
| `BENCH_SECONDS` | 10 | Length of each run. Use 30 for a steadier comparison. Compare results only between runs with the same window. |
| `MCO_ENABLE` | 0 | 1 outputs SYSCLK divided by 16 on PA8 (D7), for checking with a scope or a frequency counter. |
| `UART_REINIT_AFTER_CLOCK_CHANGE` | 1 | 0 skips the UART re-initialisation and reproduces the garbled serial output. See Experiments. |
| `DEMO_BAD_LATENCY` | 0 | 1 runs the 64 MHz preset with 0 flash wait states. Experimental and not yet tested on hardware, so treat anything it does as unverified. |

## Clock Presets

Every preset starts from the internal 16 MHz HSI oscillator. With the PLL: VCO = HSI16 / M x N, then SYSCLK = VCO / R. M is fixed at 1. HCLK and PCLK1 are not divided, so they run at SYSCLK.

| SYSCLK | Source | Settings | VCO | Flash wait states |
|---|---|---|---|---|
| 8 MHz | HSI16 / 2 | no PLL | n/a | 0 |
| 16 MHz | HSI16 | no PLL | n/a | 0 |
| 32 MHz | PLL | M=1, N=8, R=4 | 128 MHz | 1 |
| 48 MHz | PLL | M=1, N=6, R=2 | 96 MHz | 1 |
| 64 MHz | PLL | M=1, N=8, R=2 | 128 MHz | 2 |

Wait-state limits used here (voltage range 1): 0 wait states up to 24 MHz, 1 up to 48 MHz, 2 up to 64 MHz. `ApplyPreset()` sets the right value for each preset. Verify the limits against the STM32G0x0 reference manual (RM0454) if you change the voltage range.

## How It Works

**Waiting for B1.** `WaitForButton()` reads the idle level of the pin once at start, then waits for it to change and come back. Because it compares against the idle level instead of a fixed HIGH or LOW, it works whatever the button polarity is.

**Changing the clock safely.** `ApplyPreset()` does it in three steps:
1. Park SYSCLK on HSI at 16 MHz with 0 wait states. The PLL cannot be reconfigured while it is the system clock, and 16 MHz is legal at 0 wait states.
2. Set the HSI divider and the PLL (or turn the PLL off) for the target.
3. Switch SYSCLK to the target source and set the flash wait states for the new frequency.

Raising the clock and the wait states together in the last step keeps the flash read within what the chip allows.

**Keeping the window accurate.** The window is measured with `HAL_GetTick()`, a 1 ms tick driven by SysTick. `HAL_RCC_ClockConfig()` re-initialises the tick after a clock change, so a 10 s window is 10 s at every clock. Use a phone stopwatch on LD4 to confirm it on your own board.

**UART after a clock change.** The USART2 baud divider is calculated from the peripheral clock. After the clock changes, the old divider gives the wrong baud rate and the terminal shows garbage. `HAL_UART_Init()` recalculates it, which is what `UART_REINIT_AFTER_CLOCK_CHANGE` does.

**The workload.** A trial-division prime counter, integers only:

```c
for (uint32_t d = 3U; (d * d) <= n; d += 2U) {
    divs++;
    if ((n % d) == 0U) { isPrime = 0U; break; }
}
```

The Cortex-M0+ has no hardware divider, so every `%` is a software routine of roughly a hundred cycles. That makes the workload dominated by one well-defined operation, and the code counts every division (`divs`).

**Why divisions, not primes.** Primes found does not scale 1:1 with clock speed, because larger numbers need more divisions to test. Divisions per second is the fair throughput measure.

## Reading the Output

Example output from a 10 s sweep on a NUCLEO-G070RB (your numbers will differ):

```
=== ProtoCraft clock benchmark ===
Press B1 to start (10 s per run)
Running at 8 MHz for 10 s...
HSI/2      SYSCLK= 8 MHz  window=10 s  primes=6276  last=62501  divisions=700798
Running at 16 MHz for 10 s...
HSI        SYSCLK=16 MHz  window=10 s  primes=9708  last=101363  divisions=1370571
Running at 32 MHz for 10 s...
PLL N8 R4  SYSCLK=32 MHz  window=10 s  primes=14701  last=160183  divisions=2590137
Running at 48 MHz for 10 s...
PLL N6 R2  SYSCLK=48 MHz  window=10 s  primes=19073  last=213289  divisions=3861871
Running at 64 MHz for 10 s...
PLL N8 R2  SYSCLK=64 MHz  window=10 s  primes=22021  last=249721  divisions=4811601

SUMMARY (10 s per run)
MHz   primes     last prime   divisions/s   speed vs first
8     6276       62501        70079         1.00x
16    9708       101363       137057        1.95x
32    14701      160183       259013        3.69x
48    19073      213289       386187        5.51x
64    22021      249721       481160        6.86x
```

| Term | Meaning |
|---|---|
| Primes found | How many prime numbers were found in the window |
| Last prime | The largest prime found, which shows how far up the number line it got |
| Divisions | Each attempt to divide a candidate number by a trial divisor. This is the actual work done |
| Divisions per second | Divisions divided by the window length. The fair speed measure |
| Speed vs first | Divisions per second compared with the first preset (8 MHz). The firmware truncates this ratio to two decimals instead of rounding |
| Cycles per division | The clock speed divided by divisions per second: how many clock ticks one division costs. Not printed by the firmware, calculate it yourself |
| Wait states | Extra clock cycles the CPU waits when it reads flash. Needed above 24 MHz (1 wait state) and above 48 MHz (2 wait states) on this chip at voltage range 1 |

## Example Results

NUCLEO-G070RB, Debug build (-O0), 10 s per run:

| Clock | Primes | Last prime | Divisions/s | Speed vs 8 MHz | Cycles/division | Wait states |
|---|---|---|---|---|---|---|
| 8 MHz | 6,276 | 62,501 | 70,079 | 1.00x | 114 | 0 |
| 16 MHz | 9,708 | 101,363 | 137,057 | 1.95x | 117 | 0 |
| 32 MHz | 14,701 | 160,183 | 259,013 | 3.69x | 124 | 1 |
| 48 MHz | 19,073 | 213,289 | 386,187 | 5.51x | 124 | 1 |
| 64 MHz | 22,021 | 249,721 | 481,160 | 6.86x | 133 | 2 |

Same board and build, 30 s per run (`BENCH_SECONDS` set to 30):

| Clock | Primes | Last prime | Divisions/s | Speed vs 8 MHz | Cycles/division |
|---|---|---|---|---|---|
| 8 MHz | 12,533 | 134,399 | 67,598 | 1.00x | 118 |
| 16 MHz | 19,569 | 219,523 | 133,943 | 1.98x | 119 |
| 32 MHz | 29,780 | 347,539 | 254,569 | 3.76x | 126 |
| 48 MHz | 38,661 | 462,727 | 379,772 | 5.61x | 126 |
| 64 MHz | 44,712 | 541,837 | 474,024 | 7.01x | 135 |

### What the numbers show

- **The speed-up falls short of the clock increase.** At 64 MHz the clock is 8 times the 8 MHz clock, but the work done is about 6.9 times (10 s) to 7.0 times (30 s) as much.
- **Cycles per division rise with the clock**, from about 114 to 133 over the 10 s runs. On an ideal chip the software divide would cost the same number of cycles at any clock.
- **The rise lines up with the flash wait-state steps** (0, then 1, then 2). That is a strong hint, not proof. Reading flash with more wait states costs cycles, but this benchmark does not isolate that cause. The zero-wait-state experiment below is meant to test it.
- **A longer window costs a little more per division.** A 30 s run reaches bigger numbers, and the software divider takes slightly longer on them. Per division the effect is about 1.5 to 3.7% (the 8 MHz run shows the most). Compare results only with the same window length.
- **Diminishing returns at the top.** From 48 to 64 MHz the clock rises by 1.33 times and the work by 1.25 times.

Numbers depend on the optimisation level, the compiler version and the board. Compare runs only within the same build.

## Using Clocks Set in the CubeMX GUI (SWEEP_MODE 0)

The runtime sweep changes the clock in code. You can also set the clock in the CubeMX Clock Configuration tab, regenerate, and run the benchmark once at that clock.

1. In `benchmark.c`, set `SWEEP_MODE` to 0.
2. Open the `.ioc` file, go to the **Clock Configuration** tab and set the clock you want. Typing the target frequency into the HCLK box and pressing Enter lets CubeMX solve the PLL values for you. Check what it chose.
3. Save and click Generate Code. Confirm that CubeMX regenerated `SystemClock_Config()` in `main.c`.
4. Build, flash, press B1. The benchmark runs once and prints a line labelled `CubeMX`.

What changes in the generated code between a 16 MHz and a 64 MHz configuration (from the generated `SystemClock_Config()`):

| In SystemClock_Config() | 16 MHz | 64 MHz |
|---|---|---|
| `PLL.PLLState` | `RCC_PLL_NONE` | `RCC_PLL_ON` |
| PLL settings | none | source HSI, M=1, N=8, P=2, R=2 |
| `SYSCLKSource` | `RCC_SYSCLKSOURCE_HSI` | `RCC_SYSCLKSOURCE_PLLCLK` |
| `HAL_RCC_ClockConfig(...)` latency | `FLASH_LATENCY_0` | `FLASH_LATENCY_2` |

For other frequencies CubeMX may choose a different M, N and R combination than the runtime presets above. The result should still match the sweep for the same SYSCLK. In the test runs the GUI-configured clocks agreed with the sweep to within 0.05% in divisions per second.

## Experiments to Try

| Experiment | How | What to look for |
|---|---|---|
| Break the serial port | Set `UART_REINIT_AFTER_CLOCK_CHANGE` to 0 | Output is readable at the first clock and turns to garbage after the clock changes, because the baud divider is stale |
| Longer window | Set `BENCH_SECONDS` to 30 | Primes roughly double, cycles per division rise a little |
| Check the clock on a pin | Set `MCO_ENABLE` to 1 and measure PA8 | SYSCLK / 16, so 4 MHz at 64 MHz. Needs a scope or a frequency counter |
| Zero wait states at 64 MHz | Set `DEMO_BAD_LATENCY` to 1 | Experimental and untested. The chip may misbehave, hang or give wrong results. Do not draw conclusions without repeating it |
| Optimisation level | Build Release instead of Debug | Divisions per second change a lot. Compare only within one build |

## Customization

| Want to... | Change this |
|---|---|
| Add or change a clock preset | Edit the `presets[]` array in `benchmark.c`. Keep the PLL VCO inside the chip's allowed range and set the flash latency to match the SYSCLK |
| Use a different workload | Replace the loop in `RunWindow()`. Keep counting an operation count so you can calculate throughput |
| Use a different board in the STM32G0 family | Re-check the LED and button names and the clock limits for your part |
| Print floats | Add `-u _printf_float` to the linker flags. The default newlib-nano build supports integers and strings only. The firmware prints integers, so it does not need this |

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Nothing on the terminal | Wrong COM port, or the terminal is not at 115200 baud, 8N1. Pick the ST-LINK virtual COM port |
| Garbled text after the first run | `UART_REINIT_AFTER_CLOCK_CHANGE` is 0, or the terminal baud is not 115200 |
| Hangs at the first clock change | `HAL_RCC_OscConfig` returned an error and the code is stuck in `Error_Handler()`. Check the PLL settings, and that SYSCLK was parked on HSI first |
| No signal on PA8 | `MCO_ENABLE` is 0, or the probe is on the wrong pin. Check the Nucleo pinout |
| Speed ratio far below the clock ratio | Check wait states, and check that every run used the same optimisation level |
| Button does nothing | `B1_Pin` or `B1_GPIO_Port` differ in your generated `main.h` |
| Build error about `huart2` or `LD4_Pin` | The project was not created from the NUCLEO-G070RB board selector with default peripherals |
| Compile error on `RCC_PLLQ_DIV2` or `PLLQ` | The G070 has no PLLQ output. Remove those lines |

## Limitations

- Debug build with `-O0`. Absolute numbers will differ with optimisation.
- The workload is a single synthetic benchmark. Real firmware has a different mix of flash reads, RAM accesses and peripheral waits.
- Power at different clocks is not measured by this firmware.
- The wait-state explanation of the cycles-per-division trend is a hypothesis supported by the numbers, not a proven result.

## 📺 Video

Standalone video on the
[ProtoCraft Electronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1) YouTube channel.
Video link: coming soon.

## Repository Structure

```
Nucleo-G070RB-Clock-Speed-Prime-Benchmark/
├── firmware/
│   └── clock_speed_prime_benchmark/
│       ├── benchmark.c   # Copy into Core/Src/
│       └── benchmark.h   # Copy into Core/Inc/
└── README.md             # This file
```

## License

Released under the MIT License, see `LICENSE` at the repo root. Use it, modify it, ship it in your own projects.

## Credits

Built by **ProtoCraft Electronics**, hardware tutorials from a working hardware design engineer, no fluff, real debugging included.
More builds: `github.com/ProtoCraft-Electronics/ProtoCraft-Electronics`
YouTube: [youtube.com/@ProtoCraftElectronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1)
