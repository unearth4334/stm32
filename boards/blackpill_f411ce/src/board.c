#include "board.h"

/**
 * @file board.c
 * @brief WeAct STM32F411CE BlackPill — board init implementation.
 *
 * Enables GPIO clocks needed by the board and configures any on-board
 * hardware that doesn't belong to a specific driver.
 */

void board_init(void)
{
    /* Enable GPIO port clocks used by on-board hardware */
    BOARD_LED_GPIO_CLK_EN();
    BOARD_BTN_GPIO_CLK_EN();

    /* Additional peripheral clock enables go here as the design grows,
       e.g. __HAL_RCC_I2C1_CLK_ENABLE(), __HAL_RCC_SPI1_CLK_ENABLE(), … */
}
