#ifndef BOARD_H
#define BOARD_H

/**
 * @file board.h
 * @brief STMicroelectronics STM32L0 Discovery — board configuration layer.
 *
 * Defines pin mappings, peripheral instances, and hardware constants.
 * Only platform/src and drivers may include this header.
 * Application and services layers must never include board.h directly.
 *
 * STM32L0 Discovery board hardware summary:
 *   MCU   : STM32L053C8  (64 KB Flash, 8 KB SRAM, max 32 MHz)
 *   HSE   : 8 MHz crystal (optional, using MSI by default)
 *   LED   : PA5 (LD4, green, active-high)
 *   Button: PA0 (SW1, active-high)
 *   UART  : PA2 (TX), PA3 (RX) - USART2 (virtual COM via ST-Link)
 *   SWD   : PA13 (SWDIO), PA14 (SWCLK)
 */

#include "stm32l0xx_hal.h"

/* ---------- User LED (PA5, active-high) ---------------------------------- */
#define BOARD_LED_PORT          GPIOA
#define BOARD_LED_PIN           GPIO_PIN_5
#define BOARD_LED_GPIO_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()

/* ---------- User button (PA0, active-high) -------------------------------- */
#define BOARD_BTN_PORT          GPIOA
#define BOARD_BTN_PIN           GPIO_PIN_0
#define BOARD_BTN_GPIO_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()

/* ---------- I2C1 (PB8=SCL, PB9=SDA, AF4) ----------------------------------- */
#define BOARD_I2C1_INSTANCE     I2C1
#define BOARD_I2C1_SCL_PORT     GPIOB
#define BOARD_I2C1_SCL_PIN      GPIO_PIN_8
#define BOARD_I2C1_SDA_PORT     GPIOB
#define BOARD_I2C1_SDA_PIN      GPIO_PIN_9
#define BOARD_I2C1_GPIO_CLK_EN() __HAL_RCC_GPIOB_CLK_ENABLE()
#define BOARD_I2C1_CLK_EN()     __HAL_RCC_I2C1_CLK_ENABLE()

/* ---------- USART2 (debug / ST-Link virtual COM) ------------ */
#define BOARD_DEBUG_UART        USART2
#define BOARD_DEBUG_UART_BAUD   115200U
#define BOARD_DEBUG_UART_TX_PORT GPIOA
#define BOARD_DEBUG_UART_TX_PIN  GPIO_PIN_2
#define BOARD_DEBUG_UART_RX_PORT GPIOA
#define BOARD_DEBUG_UART_RX_PIN  GPIO_PIN_3
#define BOARD_DEBUG_UART_GPIO_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()
#define BOARD_DEBUG_UART_CLK_EN() __HAL_RCC_USART2_CLK_ENABLE()

#endif /* BOARD_H */
