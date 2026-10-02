/* ============================================================
 * ProtoCraft Electronics - STM32 Bring-Up
 * NUCLEO-G070RB: LD4 toggle on B1 press, printf over USART2 (ST-Link VCOM)
 *
 * These are the USER CODE sections only. Paste each block into the
 * matching USER CODE BEGIN/END markers in your CubeMX-generated main.c.
 * Assumes CubeMX already configured LD4 (PA5, output), B1 (PC13, input,
 * internal pull-up), and USART2 (115200 baud) via the NUCLEO-G070RB
 * board selector.
 * ============================================================ */

/* USER CODE BEGIN Includes */
#include <stdio.h>   /* Needed for printf(). __io_putchar() below is what
                        actually routes printf's output to real hardware,
                        without it printf compiles fine but goes nowhere. */
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
#define BUTTON_DEBOUNCE_MS   40U   /* How long B1 has to sit still before we
                                       trust its reading. Same value already
                                       proven on this channel's ESP32 button
                                       project, no reason to run two
                                       different debounce times for the
                                       same physical action. */

/* Debounce state: what B1 read last, and how long it's been sitting there */
static GPIO_PinState buttonLastRaw        = GPIO_PIN_SET;   /* idle = HIGH (pull-up) */
static GPIO_PinState buttonStable         = GPIO_PIN_SET;   /* the "confirmed" reading, after debounce */
static uint32_t       buttonLastChangeTick = 0U;             /* HAL_GetTick() value from the last raw change */

/* Current LED state, tracked in software so a button press can just flip it */
static GPIO_PinState  ledState            = GPIO_PIN_RESET; /* LD4 starts OFF */
/* USER CODE END PV */

/* USER CODE BEGIN PFP */
static void HandleButton(void);  /* Non-blocking button read + debounce + LED
                                     toggle, called once per main loop pass */
/* USER CODE END PFP */

/* USER CODE BEGIN WHILE */
/* Boot message: proves the board is alive and the UART link actually
   works, before anything else happens */
printf("ProtoCraft Electronics - STM32 Bring-Up - NUCLEO-G070RB online\r\n");

while (1)
{
  HandleButton();   /* Checked every loop pass. Non-blocking, so this never
                        stalls the rest of the loop waiting on a press. */
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */

/* USER CODE BEGIN 4 */

/**
  * @brief  The one function printf() actually depends on to produce
  *         visible output. STM32CubeIDE's own syscalls.c already loops
  *         over every character you print and calls this function for
  *         each one, it's just never told what to DO with that character
  *         until now. Point it at HAL_UART_Transmit on USART2, the same
  *         peripheral the board selector already wired to the ST-Link's
  *         virtual COM port, and printf output shows up in a serial
  *         terminal.
  */
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/**
  * @brief  Reads B1, debounces it, and toggles LD4 on a confirmed press.
  *         Called every loop iteration, never blocks, so it costs nothing
  *         when the button isn't being touched.
  */
static void HandleButton(void)
{
  /* Raw, unfiltered reading right now. A mechanical button "bounces" for
     a few milliseconds when pressed or released, so this can flicker
     between HIGH and LOW several times on a single physical press. */
  GPIO_PinState raw = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

  /* Every time the raw reading changes, restart the debounce timer, so
     the timer only finishes counting once the signal has actually
     settled down. */
  if (raw != buttonLastRaw)
  {
    buttonLastChangeTick = HAL_GetTick();
    buttonLastRaw = raw;
  }

  /* Only trust the reading once it's been stable for BUTTON_DEBOUNCE_MS,
     this is what filters out the bounce noise. */
  if ((HAL_GetTick() - buttonLastChangeTick) > BUTTON_DEBOUNCE_MS)
  {
    /* Reading actually changed since the last CONFIRMED state, this is a
       real, debounced transition, not bounce. */
    if (raw != buttonStable)
    {
      buttonStable = raw;

      /* B1 idles HIGH and reads LOW when pressed (active-low), so a
         transition to LOW is the moment of a fresh press. This is what
         makes it "one press = one toggle" instead of repeatedly
         toggling for as long as the button stays held down. */
      if (buttonStable == GPIO_PIN_RESET)
      {
        ledState = (ledState == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
        HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, ledState);
        printf("Button pressed -> LED state: %s\r\n",
               (ledState == GPIO_PIN_SET) ? "ON" : "OFF");
      }
    }
  }
}
/* USER CODE END 4 */
