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

/* ---------- I2C1 (PB8=SCL, PB9=SDA, AF4) ----------------------------------- */
#define BOARD_I2C1_INSTANCE     I2C1
#define BOARD_I2C1_SCL_PORT     GPIOB
#define BOARD_I2C1_SCL_PIN      GPIO_PIN_8
#define BOARD_I2C1_SDA_PORT     GPIOB
#define BOARD_I2C1_SDA_PIN      GPIO_PIN_9
#define BOARD_I2C1_GPIO_CLK_EN() __HAL_RCC_GPIOB_CLK_ENABLE()
#define BOARD_I2C1_CLK_EN()     __HAL_RCC_I2C1_CLK_ENABLE()

/* ---------- SPI1 (PA5=SCK, PA6=MISO, PA7=MOSI, AF5) ---------------------- */
#define BOARD_SPI1_INSTANCE      SPI1
#define BOARD_SPI1_SCK_PORT      GPIOA
#define BOARD_SPI1_SCK_PIN       GPIO_PIN_5
#define BOARD_SPI1_MISO_PORT     GPIOA
#define BOARD_SPI1_MISO_PIN      GPIO_PIN_6
#define BOARD_SPI1_MOSI_PORT     GPIOA
#define BOARD_SPI1_MOSI_PIN      GPIO_PIN_7
#define BOARD_SPI1_GPIO_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()
#define BOARD_SPI1_CLK_EN()      __HAL_RCC_SPI1_CLK_ENABLE()

/* ---------- UART1 (debug / ST-Link virtual COM on some boards) ------------ */
#define BOARD_DEBUG_UART        USART1
#define BOARD_DEBUG_UART_BAUD   115200U
#define BOARD_DEBUG_UART_TX_PORT GPIOA
#define BOARD_DEBUG_UART_TX_PIN  GPIO_PIN_9
#define BOARD_DEBUG_UART_RX_PORT GPIOA
#define BOARD_DEBUG_UART_RX_PIN  GPIO_PIN_10
#define BOARD_DEBUG_UART_GPIO_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()
#define BOARD_DEBUG_UART_CLK_EN() __HAL_RCC_USART1_CLK_ENABLE()
#define BOARD_DEBUG_UART_IRQn            USART1_IRQn
/* Must be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY to call FreeRTOS ISR APIs */
#define BOARD_DEBUG_UART_IRQ_PRIORITY    5U

/* ---------- USB FS device (PA11=DM, PA12=DP, AF10) ------------------------ */
#define BOARD_USB_DM_PORT        GPIOA
#define BOARD_USB_DM_PIN         GPIO_PIN_11
#define BOARD_USB_DP_PORT        GPIOA
#define BOARD_USB_DP_PIN         GPIO_PIN_12
#define BOARD_USB_GPIO_CLK_EN()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define BOARD_USB_CLK_EN()       __HAL_RCC_USB_OTG_FS_CLK_ENABLE()
#define BOARD_USB_IRQn           OTG_FS_IRQn
#define BOARD_USB_IRQ_PRIORITY   6U

/* ---------- Crystal / clock ---------------------------------------------- */
#define BOARD_HSE_FREQ_HZ       25000000U
#define BOARD_SYSCLK_FREQ_HZ    96000000U

/**
 * @brief Enable GPIO clocks and initialise board-level peripherals.
 *        Called once before platform_init() completes.
 */
void board_init(void);

#endif /* BOARD_H */
