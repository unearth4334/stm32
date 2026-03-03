#include "board.h"

#include "stm32f4xx_hal.h"

void Board_SystemClock_Config(void)
{
}

void Board_Init(void)
{
    HAL_Init();
    Board_SystemClock_Config();
}

void Board_DelayMs(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}
