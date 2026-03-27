#include "stm32l0xx_hal.h"
#include "platform/usb_composite.h"

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void USB_IRQHandler(void)
{
    platform_usb_composite_irq_handler();
}

void HardFault_Handler(void)
{
    __disable_irq();
    for (;;) {}
}
