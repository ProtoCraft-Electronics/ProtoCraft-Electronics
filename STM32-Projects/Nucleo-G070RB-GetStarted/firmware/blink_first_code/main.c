/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  *
  * Reference copy of the CubeIDE-generated main.c for this video, trimmed to
  * the parts that matter. Don't drop this file into an empty folder and
  * expect it to build — the real project (Drivers/, Core/, .project,
  * .cproject, linker script) only exists after you generate it from
  * STM32CubeIDE's Device Configuration Tool per the project README. This file
  * exists so you can see exactly which two lines were added by hand, inside
  * the USER CODE tags CubeMX won't overwrite on regeneration.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes ------------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void)
{
  /* MCU Configuration --------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE BEGIN 3 -- the two lines added in the video */
    HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);
    HAL_Delay(500);
    /* USER CODE END 3 */
  }
}

/**
  * @brief GPIO Initialization Function
  * Configures LD4 (PA5, the on-board user LED) as a push-pull output.
  * CubeMX generates this automatically once PA5 is set to GPIO_Output
  * in the Pinout & Configuration view.
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD4_GPIO_Port, &GPIO_InitStruct);
}
