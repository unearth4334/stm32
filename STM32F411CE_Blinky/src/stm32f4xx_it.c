#include "stm32f4xx_hal.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

void SysTick_Handler(void)
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
