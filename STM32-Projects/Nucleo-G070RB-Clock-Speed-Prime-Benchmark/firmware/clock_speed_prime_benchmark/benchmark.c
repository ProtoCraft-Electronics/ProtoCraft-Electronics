/*
 * benchmark.c
 * ProtoCraft Electronics - STM32 clock speed prime benchmark
 *
 * Target : NUCLEO-G070RB (STM32G070RB, Cortex-M0+, 64 MHz max, no FPU,
 *          no hardware divider, so every "%" below is a software routine)
 * Needs  : CubeIDE project created from the NUCLEO-G070RB board selector with
 *          the default peripherals (USART2 on the ST-LINK virtual COM port,
 *          LD4 user LED, B1 user button). USART2 at 115200 8N1.
 *
 * What it does
 *   Press B1. For each clock preset the firmware reconfigures the clock
 *   tree, counts primes by trial division for BENCH_SECONDS, and prints
 *   the result over the virtual COM port. LD4 is ON while a run is in
 *   progress, so a scope or stopwatch can check the window length.
 *   After the last preset a summary table is printed.
 *
 * Tested on a NUCLEO-G070RB with STM32CubeIDE (Debug build, -O0).
 */

#include "benchmark.h"
#include "main.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/* ---------------------------------------------------------------------- */
/* Configuration switches                                                  */
/* ---------------------------------------------------------------------- */
#define SWEEP_MODE                     1  /* 1 = cycle all presets, 0 = run once at the clock CubeMX configured */
#define BENCH_SECONDS                  10U /* use 30U for the final 16 MHz vs 64 MHz comparison */
#define MCO_ENABLE                     0  /* 1 = output SYSCLK / 16 on PA8 (D7) for a scope */
#define UART_REINIT_AFTER_CLOCK_CHANGE 1  /* set to 0 to reproduce the garbled serial output */
#define DEMO_BAD_LATENCY               0  /* 1 = run 64 MHz with 0 flash wait states (experimental) */

extern UART_HandleTypeDef huart2;
extern void Error_Handler(void);

/* ---------------------------------------------------------------------- */
/* Clock presets. Every one starts from the internal 16 MHz HSI.           */
/*   No PLL : SYSCLK = HSI16 / hsiDiv                                      */
/*   PLL    : VCO = HSI16 / M * N, SYSCLK = VCO / R   (M fixed at 1)       */
/*   Flash latency for voltage range 1: 0 WS <= 24 MHz, 1 WS <= 48 MHz,    */
/*   2 WS <= 64 MHz.                                                       */
/* ---------------------------------------------------------------------- */
typedef struct {
    const char *name;
    uint32_t    hsiDiv;
    uint32_t    usePll;
    uint32_t    pllN;
    uint32_t    pllR;
    uint32_t    flashLatency;
} ClockPreset;

#if DEMO_BAD_LATENCY
#define LATENCY_AT_64 FLASH_LATENCY_0
#else
#define LATENCY_AT_64 FLASH_LATENCY_2
#endif

static const ClockPreset presets[] = {
    { "HSI/2",        RCC_HSI_DIV2, 0U, 0U, 0U,            FLASH_LATENCY_0 }, /*  8 MHz */
    { "HSI",          RCC_HSI_DIV1, 0U, 0U, 0U,            FLASH_LATENCY_0 }, /* 16 MHz */
    { "PLL N8 R4",    RCC_HSI_DIV1, 1U, 8U, RCC_PLLR_DIV4, FLASH_LATENCY_1 }, /* 32 MHz */
    { "PLL N6 R2",    RCC_HSI_DIV1, 1U, 6U, RCC_PLLR_DIV2, FLASH_LATENCY_1 }, /* 48 MHz */
    { "PLL N8 R2",    RCC_HSI_DIV1, 1U, 8U, RCC_PLLR_DIV2, LATENCY_AT_64   }, /* 64 MHz */
};
#define NUM_PRESETS (sizeof(presets) / sizeof(presets[0]))

typedef struct {
    uint32_t freq;    /* Hz, from HAL_RCC_GetSysClockFreq() */
    uint32_t primes;  /* primes found inside the window     */
    uint32_t last;    /* largest prime found                */
    uint32_t divs;    /* trial divisions performed          */
} Result;

/* ---------------------------------------------------------------------- */
/* Serial helper (no float, no malloc)                                     */
/* ---------------------------------------------------------------------- */
static void Print(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) {
        if (n >= (int)sizeof buf) {
            n = (int)sizeof buf - 1;
        }
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)n, 200U);
    }
}

/* ---------------------------------------------------------------------- */
/* Button: idle level is read at start, so pin polarity does not matter    */
/* ---------------------------------------------------------------------- */
static void WaitForButton(void)
{
    GPIO_PinState idle = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
    while (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == idle) {
        HAL_Delay(10U);
    }
    HAL_Delay(50U);
    while (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) != idle) {
        HAL_Delay(10U);
    }
}

/* ---------------------------------------------------------------------- */
/* Move the whole clock tree to a preset                                   */
/* ---------------------------------------------------------------------- */
static void ApplyPreset(const ClockPreset *p)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* Step 1: park SYSCLK on HSI so the PLL and HSI divider can be changed.
       16 MHz is legal at 0 wait states, so the latency drops with it. */
    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }

    /* Step 2: set the HSI divider and the PLL for the target */
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSIDiv              = p->hsiDiv;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    if (p->usePll) {
        osc.PLL.PLLState  = RCC_PLL_ON;
        osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
        osc.PLL.PLLM      = RCC_PLLM_DIV1;
        osc.PLL.PLLN      = p->pllN;
        osc.PLL.PLLP      = RCC_PLLP_DIV2;
        osc.PLL.PLLR      = p->pllR;
    } else {
        osc.PLL.PLLState  = RCC_PLL_OFF;
    }
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    /* Step 3: switch SYSCLK to the target source with the right wait states */
    clk.SYSCLKSource = p->usePll ? RCC_SYSCLKSOURCE_PLLCLK : RCC_SYSCLKSOURCE_HSI;
    if (HAL_RCC_ClockConfig(&clk, p->flashLatency) != HAL_OK) {
        Error_Handler();
    }

#if UART_REINIT_AFTER_CLOCK_CHANGE
    /* USART2 baud divider is computed from PCLK. A new clock needs a new divider. */
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
#endif

#if MCO_ENABLE
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_16);
#endif
}

/* ---------------------------------------------------------------------- */
/* The workload: trial-division prime counter, integer only                */
/* ---------------------------------------------------------------------- */
static Result RunWindow(void)
{
    Result r;
    uint32_t primes = 1U;   /* counts the prime 2 */
    uint32_t last   = 2U;
    uint32_t divs   = 0U;
    uint32_t n      = 3U;

    r.freq = HAL_RCC_GetSysClockFreq();

    /* Blocking print finishes before the window starts, so it does not eat into the 10 s */
    Print("Running at %lu MHz for %lu s...\r\n",
          (unsigned long)(r.freq / 1000000UL),
          (unsigned long)BENCH_SECONDS);

    uint32_t end = HAL_GetTick() + (BENCH_SECONDS * 1000U);
    HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_SET);

    while ((int32_t)(HAL_GetTick() - end) < 0) {
        uint32_t isPrime = 1U;
        for (uint32_t d = 3U; (d * d) <= n; d += 2U) {
            divs++;
            if ((n % d) == 0U) {
                isPrime = 0U;
                break;
            }
        }
        if (isPrime) {
            primes++;
            last = n;
        }
        n += 2U;
    }

    HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);

    r.primes = primes;
    r.last   = last;
    r.divs   = divs;
    return r;
}

static void PrintRun(const char *label, const Result *r)
{
    Print("%-10s SYSCLK=%2lu MHz  window=%lu s  primes=%lu  last=%lu  divisions=%lu\r\n",
          label,
          (unsigned long)(r->freq / 1000000UL),
          (unsigned long)BENCH_SECONDS,
          (unsigned long)r->primes,
          (unsigned long)r->last,
          (unsigned long)r->divs);
}

static void PrintSummary(const Result *res, uint32_t count)
{
    uint32_t base = res[0].divs;
    Print("\r\nSUMMARY (%lu s per run)\r\n", (unsigned long)BENCH_SECONDS);
    Print("MHz   primes     last prime   divisions/s   speed vs first\r\n");
    for (uint32_t i = 0U; i < count; i++) {
        uint32_t dps    = res[i].divs / BENCH_SECONDS;
        uint32_t x100   = (base != 0U) ? (uint32_t)(((uint64_t)res[i].divs * 100ULL) / base) : 0U;
        Print("%-5lu %-10lu %-12lu %-13lu %lu.%02lux\r\n",
              (unsigned long)(res[i].freq / 1000000UL),
              (unsigned long)res[i].primes,
              (unsigned long)res[i].last,
              (unsigned long)dps,
              (unsigned long)(x100 / 100UL),
              (unsigned long)(x100 % 100UL));
    }
    Print("\r\n");
}

/* ---------------------------------------------------------------------- */
void Benchmark_Run(void)
{
    Result results[NUM_PRESETS];

    Print("\r\n=== ProtoCraft clock benchmark ===\r\n");

    for (;;) {
        Print("Press B1 to start (%lu s per run)\r\n", (unsigned long)BENCH_SECONDS);
        WaitForButton();

#if SWEEP_MODE
        for (uint32_t i = 0U; i < NUM_PRESETS; i++) {
            ApplyPreset(&presets[i]);
            HAL_Delay(50U); /* let the UART and MCO pin settle */
            results[i] = RunWindow();
            PrintRun(presets[i].name, &results[i]);
        }
        PrintSummary(results, NUM_PRESETS);
#else
        Result r = RunWindow();
        PrintRun("CubeMX", &r);
#endif
    }
}
