#include "app_blinky.h"
#include "led_service.h"
#include <stdint.h>

/* Blink period defined at compile-time via app_config.h if present */
#ifndef BLINK_PERIOD_MS
#define BLINK_PERIOD_MS 500U
#endif

void app_blinky_init(void)
{
    led_service_init();
}

void app_blinky_run(void)
{
    led_service_toggle();
    /* Delay is provided by the platform clock service; kept here as a stub.
       Replace with an RTOS delay or a non-blocking state machine as needed. */
    extern void platform_delay_ms(uint32_t ms);
    platform_delay_ms(BLINK_PERIOD_MS);
}
