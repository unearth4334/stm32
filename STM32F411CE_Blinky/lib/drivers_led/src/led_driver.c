#include "led_driver.h"

#include "board_config.h"
#include "stm32f4xx_hal.h"

static void LedDriver_WritePin(GPIO_PinState state)
{
    HAL_GPIO_WritePin(BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN, state);
}

void LedDriver_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    BOARD_LED_GPIO_CLK_ENABLE();

    gpio_init.Pin = BOARD_LED_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(BOARD_LED_GPIO_PORT, &gpio_init);
    LedDriver_Off();
}

void LedDriver_On(void)
{
    LedDriver_WritePin(BOARD_LED_ACTIVE_STATE);
}

void LedDriver_Off(void)
{
    LedDriver_WritePin(BOARD_LED_INACTIVE_STATE);
}

void LedDriver_Toggle(void)
{
    HAL_GPIO_TogglePin(BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN);
}
