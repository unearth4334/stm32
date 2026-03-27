/**
 * @file platform_clock.c
 * @brief STM32L0 clock management
 */

#include "stm32l0xx_hal.h"

uint32_t Platform_GetSysClkFreq(void)
{
    return HAL_RCC_GetSysClockFreq();
}

uint32_t Platform_GetHClkFreq(void)
{
    return HAL_RCC_GetHCLKFreq();
}

uint32_t Platform_GetAPB1ClkFreq(void)
{
    return HAL_RCC_GetPCLK1Freq();
}

uint32_t Platform_GetAPB2ClkFreq(void)
{
    return HAL_RCC_GetPCLK2Freq();
}
