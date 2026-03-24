#ifndef PLATFORM_CLOCK_H
#define PLATFORM_CLOCK_H

/**
 * @file clock.h
 * @brief Platform clock / delay abstraction — platform layer.
 *
 * Implementation: platform/src/stm32f4/hal/platform_clock.c
 */

#include <stdint.h>

/** Return the current system tick count in milliseconds. */
uint32_t platform_get_tick_ms(void);

/** Blocking millisecond delay (uses HAL_Delay internally). */
void platform_delay_ms(uint32_t ms);

#endif /* PLATFORM_CLOCK_H */
