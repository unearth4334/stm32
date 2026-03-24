#include "stm32f4xx_hal.h"

/*
 * HAL_Delay depends on uwTick, which is advanced from SysTick IRQ.
 * Without this handler, application code appears to run once then stall.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
