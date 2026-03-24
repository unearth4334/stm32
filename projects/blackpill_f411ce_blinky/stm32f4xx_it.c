#include "stm32f4xx_hal.h"

#if defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"

extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
#endif

#if defined(USE_FREERTOS)
void SVC_Handler(void)
{
    vPortSVCHandler();
}

void PendSV_Handler(void)
{
    xPortPendSVHandler();
}
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
