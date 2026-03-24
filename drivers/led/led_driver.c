#include "drivers/led/led_driver.h"
#include "platform/gpio.h"
#include "board.h"   /* BOARD_LED_PORT, BOARD_LED_PIN */

static led_driver_state_t s_state = LED_DRIVER_STATE_OFF;

void led_driver_init(void)
{
    platform_gpio_init_output(BOARD_LED_PORT, BOARD_LED_PIN,
                              PLATFORM_GPIO_SPEED_LOW);
    platform_gpio_write(BOARD_LED_PORT, BOARD_LED_PIN, 0U);
    s_state = LED_DRIVER_STATE_OFF;
}

void led_driver_set(led_driver_state_t state)
{
    /* BlackPill user LED (PC13) is active-low */
    uint8_t pin_val = (state == LED_DRIVER_STATE_ON) ? 0U : 1U;
    platform_gpio_write(BOARD_LED_PORT, BOARD_LED_PIN, pin_val);
    s_state = state;
}

void led_driver_toggle(void)
{
    led_driver_set((s_state == LED_DRIVER_STATE_ON)
                   ? LED_DRIVER_STATE_OFF
                   : LED_DRIVER_STATE_ON);
}
