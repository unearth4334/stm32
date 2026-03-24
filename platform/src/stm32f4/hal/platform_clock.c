/**
 * @file platform_clock.c
 * @brief STM32F4 HAL implementation of the platform clock/delay abstraction.
 */

#include "platform/clock.h"
#include "stm32f4xx_hal.h"

uint32_t platform_get_tick_ms(void)
{
    return HAL_GetTick();
}

void platform_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}
