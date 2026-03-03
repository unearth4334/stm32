#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

void Board_Init(void);
void Board_DelayMs(uint32_t delay_ms);
void Board_SystemClock_Config(void);

HAL_StatusTypeDef Board_I2cInit(void);
I2C_HandleTypeDef *Board_I2cHandle(void);

HAL_StatusTypeDef Board_UartInit(void);
UART_HandleTypeDef *Board_UartHandle(void);

#endif
