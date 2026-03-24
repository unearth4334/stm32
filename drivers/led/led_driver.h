#ifndef LED_DRIVER_H
#define LED_DRIVER_H

/**
 * @file led_driver.h
 * @brief LED driver — drivers layer.
 *
 * Hardware-agnostic. Uses platform/gpio.h — never includes STM32 headers
 * directly (anti-pattern: drivers including HAL).
 */

typedef enum {
    LED_DRIVER_STATE_OFF = 0,
    LED_DRIVER_STATE_ON  = 1,
} led_driver_state_t;

/** Initialise the LED GPIO via the platform GPIO abstraction. */
void led_driver_init(void);

/** Set LED to an explicit state. */
void led_driver_set(led_driver_state_t state);

/** Toggle LED state. */
void led_driver_toggle(void);

#endif /* LED_DRIVER_H */
