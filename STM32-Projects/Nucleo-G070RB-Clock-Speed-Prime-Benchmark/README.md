# STM32 Clock Speed Prime Benchmark

Change the clock tree on an STM32G0 and measure what it does with a prime number benchmark.

Part of the **ProtoCraft Electronics** "STM32 Bring-Up" series.

Tested on a NUCLEO-G070RB with STM32CubeIDE, Debug build (-O0).

## Hardware

- NUCLEO-G070RB (STM32G070RB, Cortex-M0+, 64 MHz max, no FPU, no hardware divider)
- USB cable (power, programming, and the serial port)
- Optional: multimeter on the IDD jumper. No scope is needed: a phone stopwatch on LD4 checks the window.

## Software

- STM32CubeIDE
- A serial terminal at 115200 baud, 8N1 (the ST-LINK virtual COM port)

## Setup

1. In CubeIDE, create a new project and pick NUCLEO-G070RB in the Board Selector. Accept the default peripheral initialisation. This gives you USART2 on the virtual COM port, the LD4 user LED, and the B1 user button.
2. Copy `firmware/clock_speed_prime_benchmark/benchmark.h` into your project's `Core/Inc/`.
3. Copy `firmware/clock_speed_prime_benchmark/benchmark.c` into your project's `Core/Src/`.
4. In `Core/Src/main.c`, add inside the USER CODE blocks:

```c
/* USER CODE BEGIN Includes */
#include "benchmark.h"
/* USER CODE END Includes */
```

```c
/* USER CODE BEGIN 2 */
Benchmark_Run();
/* USER CODE END 2 */
```

5. Build, flash, open the terminal, and press B1.

## Switches in benchmark.c

| Define | Default | What it does |
|---|---|---|
| `SWEEP_MODE` | 1 | 1 runs all five presets. 0 runs once at whatever clock CubeMX configured, for the GUI-driven demo. |
| `BENCH_SECONDS` | 10 | Length of each window. Use 30 for the 16 vs 64 MHz comparison. |
| `MCO_ENABLE` | 0 | 1 outputs SYSCLK divided by 16 on PA8 (D7), for checking with a scope. |
| `UART_REINIT_AFTER_CLOCK_CHANGE` | 1 | 0 reproduces the garbled serial output. |
| `DEMO_BAD_LATENCY` | 0 | 1 runs 64 MHz with 0 flash wait states. Experimental. |

## Clock presets

| SYSCLK | Source | Settings | Flash wait states |
|---|---|---|---|
| 8 MHz | HSI16 / 2 | no PLL | 0 |
| 16 MHz | HSI16 | no PLL | 0 |
| 32 MHz | PLL | M=1, N=8, R=4 | 1 |
| 48 MHz | PLL | M=1, N=6, R=2 | 1 |
| 64 MHz | PLL | M=1, N=8, R=2 | 2 |

## Output format

The format below is an example only. Numbers will differ on your board.

```
=== ProtoCraft clock benchmark ===
Press B1 to start (10 s per run)
HSI/2      SYSCLK= 8 MHz  window=10 s  primes=...  last=...  divisions=...
...
SUMMARY (10 s per run)
MHz   primes     last prime   divisions/s   speed vs first
...
```

## How the benchmark works

It counts primes by trial division for a fixed window and reports three things: primes found, the largest prime, and the number of trial divisions. Primes found does not scale 1:1 with clock speed, because bigger numbers cost more divisions to test. Trial divisions per second is the throughput measure that tracks the clock.

LD4 is on while a run is in progress. Use a phone stopwatch to check the window length.

## Understanding the output

| Term | Meaning |
|---|---|
| Primes found | How many prime numbers were found in the window |
| Last prime | The largest prime found, which shows how far up the number line it got |
| Divisions | Each attempt to divide a candidate number by a trial divisor. This is the actual work done. |
| Divisions per second | Divisions divided by the window length. The fair speed measure, since bigger numbers cost more divisions to test. |
| Speed vs first | Divisions per second compared with the first preset (8 MHz) |
| Cycles per division | The clock speed divided by divisions per second: how many clock ticks one division costs. The Cortex-M0+ has no hardware divider, so a division is a software routine of about 114 cycles at 8 MHz. |
| Wait states | Extra clock cycles the CPU waits when it reads flash. Needed above 24 MHz (1 wait state) and above 48 MHz (2 wait states) on this chip. |

Because the divide routine takes the same steps at any clock, cycles per division would be constant on an ideal chip. In practice it rises slightly with clock speed. Part of that comes from bigger numbers (the software divider takes a little longer on them) and the rest lines up with the flash wait-state steps.

## Example results

NUCLEO-G070RB, Debug build (-O0), 10 s per run:

| Clock | Primes | Last prime | Divisions/s | Speed vs 8 MHz | Cycles/division | Wait states |
|---|---|---|---|---|---|---|
| 8 MHz | 6,276 | 62,501 | 70,079 | 1.00x | 114 | 0 |
| 16 MHz | 9,708 | 101,363 | 137,057 | 1.96x | 117 | 0 |
| 32 MHz | 14,701 | 160,183 | 259,013 | 3.70x | 124 | 1 |
| 48 MHz | 19,073 | 213,289 | 386,187 | 5.51x | 124 | 1 |
| 64 MHz | 22,021 | 249,721 | 481,160 | 6.87x | 133 | 2 |

Same board and build, 30 s per run (`BENCH_SECONDS` set to 30):

| Clock | Primes | Last prime | Divisions/s | Speed vs 8 MHz | Cycles/division |
|---|---|---|---|---|---|
| 8 MHz | 12,533 | 134,399 | 67,598 | 1.00x | 118 |
| 16 MHz | 19,569 | 219,523 | 133,943 | 1.98x | 119 |
| 32 MHz | 29,780 | 347,539 | 254,569 | 3.77x | 126 |
| 48 MHz | 38,661 | 462,727 | 379,772 | 5.62x | 126 |
| 64 MHz | 44,712 | 541,837 | 474,024 | 7.01x | 135 |

A longer window reaches bigger numbers, so each division costs about 1.5 to 3.7% more cycles. Compare runs only with the same window length.

Numbers will differ with the optimisation level and the board. Compare runs only within the same build.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Garbled serial after the first run | `UART_REINIT_AFTER_CLOCK_CHANGE` is 0, or the terminal baud is not 115200 |
| Hangs at the first clock change | `HAL_RCC_OscConfig` returned an error. Check the PLL settings and that SYSCLK was parked on HSI first. |
| No signal on PA8 | `MCO_ENABLE` is 0, or the probe is on the wrong pin. Check the Nucleo pinout. |
| Speed ratio far below the clock ratio | Check wait states, and check that every run used the same optimisation level |
| Button does nothing | `B1_Pin` or `B1_GPIO_Port` names differ in your generated `main.h` |

## License

Code: MIT. Hardware files (if any are added): CC BY 4.0. Same as the repo root.
