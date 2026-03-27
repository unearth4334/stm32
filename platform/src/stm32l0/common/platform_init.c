/**
 * @file platform_init.c
 * @brief STM32L0 platform initialization
 *
 * Initializes system clocks, peripheral clocks, and other hardware resources.
 */

#include "stm32l0xx_hal.h"
#include "board.h"

/**
 * @brief SystemClock_Config
 *
 * Configures the system clock.
 * STM32L0 operates at max 32 MHz using MSI (Multi-Speed Internal oscillator).
 *
 * Configuration:
 *   - SYSCLK  : 32 MHz (from MSI)
 *   - HCLK    : 32 MHz
 *   - PCLK1   : 32 MHz
 *   - PCLK2   : 32 MHz
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configure the main internal regulator output voltage */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;  /* 32 MHz */
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief Error_Handler
 *
 * Function executed in case of error occurrence.
 * Implemented as a while loop that toggles the LED.
 */
void Error_Handler(void)
{
    /* Disable all interrupts */
    __disable_irq();

    /* Toggle LED in a loop */
    while (1)
    {
        HAL_GPIO_TogglePin(BOARD_LED_PORT, BOARD_LED_PIN);
        HAL_Delay(100);
    }
}

/*
 * Called from main() early during startup.
 */
void platform_init(void)
{
    HAL_Init();
    SystemClock_Config();
}
