#include "app_blinky.h"

#include "board.h"
#include "board_config.h"
#include "led_driver.h"

void App_Blinky_Run(void)
{
    Board_Init();
    LedDriver_Init();

    while (1)
    {
        LedDriver_Toggle();
        Board_DelayMs(BOARD_BLINK_TOGGLE_PERIOD_MS);
    }
}
