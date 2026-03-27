/**
 * @file stm32l0xx_it.c
 * @brief STM32L0 Interrupt handlers.
 *
 * Implements required interrupt service routines for STM32L0.
 */

#include "stm32l0xx_hal.h"

/*
 * HAL_Delay depends on uwTick, which is advanced from SysTick IRQ.
 * Without this handler, application code appears to run once then stall.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
