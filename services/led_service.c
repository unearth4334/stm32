#include "led_service.h"
#include "led/led_driver.h"

void led_service_init(void)
{
    led_driver_init();
}

void led_service_on(void)
{
    led_driver_set(LED_DRIVER_STATE_ON);
}

void led_service_off(void)
{
    led_driver_set(LED_DRIVER_STATE_OFF);
}

void led_service_toggle(void)
{
    led_driver_toggle();
}
