/*
 * benchmark.h
 * ProtoCraft Electronics - STM32 clock speed prime benchmark
 * Target: NUCLEO-G070RB (STM32G070RB, Cortex-M0+, max 64 MHz)
 */
#ifndef BENCHMARK_H
#define BENCHMARK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Never returns. Call once from USER CODE BEGIN 2 in main.c. */
void Benchmark_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* BENCHMARK_H */
