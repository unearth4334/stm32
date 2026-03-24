#ifndef BOARD_H
#define BOARD_H

/**
 * @file board.h
 * @brief WeAct STM32F411CE BlackPill — board configuration layer.
 *
 * Defines pin mappings, peripheral instances, and hardware constants.
 * Only platform/src and drivers may include this header.
 * Application and services layers must never include board.h directly.
 *
 * BlackPill v3.1 hardware summary:
 *   MCU   : STM32F411CEU6  (512 KB Flash, 128 KB SRAM, max 100 MHz)
 *   HSE   : 25 MHz crystal
 *   LED   : PC13, active-low
 *   Button: PA0,  active-high (KEY)
 *   USB   : PA11 (DM), PA12 (DP)
 *   SWD   : PA13 (SWDIO), PA14 (SWCLK)
 */

#include "stm32f4xx_hal.h"

/* ---------- User LED (PC13, active-low) ---------------------------------- */
#define BOARD_LED_PORT          GPIOC
#define BOARD_LED_PIN           GPIO_PIN_13
#define BOARD_LED_GPIO_CLK_EN() __HAL_RCC_GPIOC_CLK_ENABLE()

/* ---------- User button (PA0, active-high) -------------------------------- */
#define BOARD_BTN_PORT          GPIOA
#define BOARD_BTN_PIN           GPIO_PIN_0
#define BOARD_BTN_GPIO_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()

/* ---------- UART1 (debug / ST-Link virtual COM on some boards) ------------ */
#define BOARD_DEBUG_UART        USART1
#define BOARD_DEBUG_UART_BAUD   115200U

/* ---------- Crystal / clock ---------------------------------------------- */
#define BOARD_HSE_FREQ_HZ       25000000U
#define BOARD_SYSCLK_FREQ_HZ   100000000U

/**
 * @brief Enable GPIO clocks and initialise board-level peripherals.
 *        Called once before platform_init() completes.
 */
void board_init(void);

#endif /* BOARD_H */
