#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

void Board_Init(void);
void Board_DelayMs(uint32_t delay_ms);
void Board_SystemClock_Config(void);

#endif
