/**
 * @file platform_init.c
 * @brief Low-level platform initialisation for STM32F411CE.
 *
 * Enables FPU, configures SysTick, and sets up the system clock to 96 MHz
 * using the 25 MHz HSE on the WeAct BlackPill.
 *
 * Clock tree:
 *   HSE  25 MHz → PLL (M=25, N=192, P=2) → SYSCLK 96 MHz
 *   AHB  /1  → 96 MHz  (HCLK)
 *   APB1 /2  → 48 MHz
 *   APB2 /1  → 96 MHz
 *   USB  /4  → 48 MHz
 */

#include "stm32f4xx_hal.h"

/* Called from startup before main() via HAL_Init → SystemClock_Config */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* Enable power controller clock, set voltage scale 1 */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 25;
    osc.PLL.PLLN       = 192;
    osc.PLL.PLLP       = RCC_PLLP_DIV2;
    osc.PLL.PLLQ       = 4;
    HAL_RCC_OscConfig(&osc);

    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                       | RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3);
}

void platform_init(void)
{
    HAL_Init();
    SystemClock_Config();
}
