#include "app_blinky.h"

#include "error_config.h"
#include "stm32f4xx_hal.h"

int main(void)
{
    App_Blinky_Run();

    while (1)
    {
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
