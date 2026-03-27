#include "stm32f4xx_hal.h"
#include "platform/usb_composite.h"

#if defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"

extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);

void SVC_Handler(void)   { vPortSVCHandler(); }
void PendSV_Handler(void) { xPortPendSVHandler(); }
#endif

void SysTick_Handler(void)
{
#if defined(USE_FREERTOS)
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
#endif
    HAL_IncTick();
}

void OTG_FS_IRQHandler(void)
{
    platform_usb_composite_irq_handler();
}

void HardFault_Handler(void)
{
    __disable_irq();
    for (;;) {}
}
