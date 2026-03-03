#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "stm32f4xx_hal.h"

#define BOARD_LED_GPIO_PORT                  GPIOC
#define BOARD_LED_GPIO_PIN                   GPIO_PIN_13
#define BOARD_LED_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOC_CLK_ENABLE()
#define BOARD_LED_ACTIVE_STATE               GPIO_PIN_RESET
#define BOARD_LED_INACTIVE_STATE             GPIO_PIN_SET

#define BOARD_BLINK_TOGGLE_PERIOD_MS         (500U)

#define BOARD_I2C_INSTANCE                   I2C1
#define BOARD_I2C_TIMING_HZ                  (100000U)
#define BOARD_I2C_TIMEOUT_MS                 (100U)

#define BOARD_I2C_GPIO_PORT                  GPIOB
#define BOARD_I2C_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOB_CLK_ENABLE()
#define BOARD_I2C_SCL_PIN                    GPIO_PIN_8
#define BOARD_I2C_SDA_PIN                    GPIO_PIN_9
#define BOARD_I2C_AF                         GPIO_AF4_I2C1

#define BOARD_I2C_CLK_ENABLE()               __HAL_RCC_I2C1_CLK_ENABLE()

#define BOARD_STS40_I2C_ADDRESS              (0x44U)
#define BOARD_STS40_POLL_PERIOD_MS           (2000U)

#define BOARD_CONSOLE_UART_INSTANCE          USART2
#define BOARD_CONSOLE_UART_BAUDRATE          (115200U)
#define BOARD_CONSOLE_UART_TIMEOUT_MS        (10U)

#define BOARD_CONSOLE_UART_GPIO_PORT         GPIOA
#define BOARD_CONSOLE_UART_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define BOARD_CONSOLE_UART_TX_PIN            GPIO_PIN_2
#define BOARD_CONSOLE_UART_RX_PIN            GPIO_PIN_3
#define BOARD_CONSOLE_UART_AF                GPIO_AF7_USART2

#define BOARD_CONSOLE_UART_CLK_ENABLE()      __HAL_RCC_USART2_CLK_ENABLE()

#endif
