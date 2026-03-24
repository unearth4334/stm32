#ifndef LED_SERVICE_H
#define LED_SERVICE_H

/**
 * @file led_service.h
 * @brief LED service — services layer.
 *
 * Wraps the led_driver with higher-level semantics (on/off/toggle/blink).
 * Depends on drivers/led, never on platform/ or board/ directly.
 */

/** Initialise the LED service (calls led_driver_init internally). */
void led_service_init(void);

/** Turn the user LED on. */
void led_service_on(void);

/** Turn the user LED off. */
void led_service_off(void);

/** Toggle the user LED. */
void led_service_toggle(void);

#endif /* LED_SERVICE_H */
