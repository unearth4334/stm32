#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "stm32f4xx_hal.h"

#define BOARD_LED_GPIO_PORT                  GPIOC
#define BOARD_LED_GPIO_PIN                   GPIO_PIN_13
#define BOARD_LED_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOC_CLK_ENABLE()
#define BOARD_LED_ACTIVE_STATE               GPIO_PIN_RESET
#define BOARD_LED_INACTIVE_STATE             GPIO_PIN_SET

#define BOARD_BLINK_TOGGLE_PERIOD_MS         (500U)

#endif
