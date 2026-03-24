#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

/**
 * @file gpio.h
 * @brief Platform GPIO abstraction — platform layer.
 *
 * All drivers use this API.  The implementation lives in
 * platform/src/stm32f4/hal/platform_gpio.c (HAL) or
 * platform/src/stm32f4/ll/platform_gpio_ll.c (LL, for ISR-critical paths).
 *
 * Port/pin identifiers are kept as void* + uint32_t so the platform header
 * itself never needs to include STM32 headers — only the .c impl does.
 */

#include <stdint.h>

typedef enum {
    PLATFORM_GPIO_SPEED_LOW    = 0,
    PLATFORM_GPIO_SPEED_MEDIUM = 1,
    PLATFORM_GPIO_SPEED_HIGH   = 2,
    PLATFORM_GPIO_SPEED_VHIGH  = 3,
} platform_gpio_speed_t;

/**
 * @brief Initialise a GPIO pin as push-pull output.
 * @param port  Pointer to GPIO port (e.g. GPIOC).
 * @param pin   Pin mask (e.g. GPIO_PIN_13).
 * @param speed Output speed.
 */
void platform_gpio_init_output(void *port, uint32_t pin,
                               platform_gpio_speed_t speed);

/**
 * @brief Initialise a GPIO pin as floating input.
 */
void platform_gpio_init_input(void *port, uint32_t pin);

/**
 * @brief Write a digital value to a GPIO output pin.
 * @param value  0 = low, non-zero = high.
 */
void platform_gpio_write(void *port, uint32_t pin, uint8_t value);

/**
 * @brief Read a GPIO pin.
 * @return 0 or 1.
 */
uint8_t platform_gpio_read(void *port, uint32_t pin);

#endif /* PLATFORM_GPIO_H */
