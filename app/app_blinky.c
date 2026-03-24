#include "app_blinky.h"
#include "led_service.h"
#include "osal/delay.h"

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
    osal_delay_ms(BLINK_PERIOD_MS);
}
