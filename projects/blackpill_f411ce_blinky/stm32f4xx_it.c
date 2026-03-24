#include "stm32f4xx_hal.h"

#if defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

/*
 * HAL_Delay depends on uwTick, which is advanced from SysTick IRQ.
 * Without this handler, application code appears to run once then stall.
 */
void SysTick_Handler(void)
{
#if defined(USE_FREERTOS)
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
#endif

    HAL_IncTick();
}
