/**
 * @file platform_gpio.c
 * @brief STM32F4 HAL implementation of the platform GPIO abstraction.
 *
 * Only this file includes stm32f4xx_hal.h — drivers and above never do.
 * Use the LL variant (platform/src/stm32f4/ll/platform_gpio_ll.c) for
 * performance-critical / ISR paths.
 */

#include "platform/gpio.h"
#include "stm32f4xx_hal.h"

void platform_gpio_init_output(void *port, uint32_t pin,
                               platform_gpio_speed_t speed)
{
    GPIO_InitTypeDef cfg = {0};

    /* Map abstract speed to HAL speed */
    const uint32_t hal_speed_map[] = {
        GPIO_SPEED_FREQ_LOW,
        GPIO_SPEED_FREQ_MEDIUM,
        GPIO_SPEED_FREQ_HIGH,
        GPIO_SPEED_FREQ_VERY_HIGH,
    };

    cfg.Pin   = pin;
    cfg.Mode  = GPIO_MODE_OUTPUT_PP;
    cfg.Pull  = GPIO_NOPULL;
    cfg.Speed = hal_speed_map[(uint8_t)speed & 0x03U];

    HAL_GPIO_Init((GPIO_TypeDef *)port, &cfg);
}

void platform_gpio_init_input(void *port, uint32_t pin)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin  = pin;
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_NOPULL;
    HAL_GPIO_Init((GPIO_TypeDef *)port, &cfg);
}

void platform_gpio_write(void *port, uint32_t pin, uint8_t value)
{
    HAL_GPIO_WritePin((GPIO_TypeDef *)port, pin,
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t platform_gpio_read(void *port, uint32_t pin)
{
    return (uint8_t)HAL_GPIO_ReadPin((GPIO_TypeDef *)port, pin);
}
