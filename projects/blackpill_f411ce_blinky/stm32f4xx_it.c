#include "stm32f4xx_hal.h"
#include "console/console.h"
#include "platform/uart.h"
#include "platform/usb_cdc.h"

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

void USART1_IRQHandler(void)
{
    platform_uart_irq_handler(platform_uart_debug_handle());
}

void OTG_FS_IRQHandler(void)
{
    platform_usb_cdc_irq_handler();
}

void HardFault_Handler(void)
{
    console_record_fault("hardfault", 0x48465254U, SCB->CFSR, SCB->HFSR);
    console_log_panic("fault", "hardfault cfsr=0x%08lX hfsr=0x%08lX",
                      (unsigned long)SCB->CFSR,
                      (unsigned long)SCB->HFSR);

    __disable_irq();
    for (;;)
    {
    }
}
