/**
 * @file board.c
 * @brief STM32L0 Discovery board init implementation.
 *
 * Enables GPIO clocks needed by the board and configures on-board hardware.
 */

#include "board.h"

void board_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    BOARD_LED_GPIO_CLK_EN();
    BOARD_BTN_GPIO_CLK_EN();
    BOARD_I2C1_GPIO_CLK_EN();
    BOARD_DEBUG_UART_GPIO_CLK_EN();

    /* -------- Configure User LED (PA5, active-high) -------- */
    GPIO_InitStruct.Pin = BOARD_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BOARD_LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_PIN_RESET);

    /* -------- Configure User Button (PA0, active-high) -------- */
    GPIO_InitStruct.Pin = BOARD_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BOARD_BTN_PORT, &GPIO_InitStruct);

    /* -------- Configure Debug UART (PA2=TX, PA3=RX) -------- */
    GPIO_InitStruct.Pin = BOARD_DEBUG_UART_TX_PIN | BOARD_DEBUG_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART2;
    HAL_GPIO_Init(BOARD_DEBUG_UART_TX_PORT, &GPIO_InitStruct);

    /* -------- Configure I2C1 (PB8=SCL, PB9=SDA) -------- */
    GPIO_InitStruct.Pin = BOARD_I2C1_SCL_PIN | BOARD_I2C1_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(BOARD_I2C1_SCL_PORT, &GPIO_InitStruct);
}
