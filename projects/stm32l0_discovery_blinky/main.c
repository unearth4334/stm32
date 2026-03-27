/**
 * @file main.c
 * @brief Entry point — stm32l0_discovery_blinky project.
 *
 * Dependency chain (read bottom-up):
 *   vendor (STM32CubeL0 / CMSIS)
 *     └─ board   (boards/stm32l0_discovery)
 *         └─ platform (platform/stm32l0/hal)
 *             └─ drivers (drivers/led)
 *                 └─ services (services/led_service)
 *                     └─ app (app/app_blinky)  ← main calls this
 *
 * main() itself is kept minimal: initialise, then loop.
 */

#include "board.h"
#include "app_blinky.h"

/* Defined in platform/src/stm32l0/common/platform_init.c */
extern void platform_init(void);

int main(void)
{
    platform_init();   /* HAL_Init + SystemClock_Config              */
    board_init();      /* Enable GPIO clocks, configure board hw     */

    app_blinky_init(); /* Init services → drivers → platform GPIO   */

    while (1)
    {
        app_blinky_run(); /* Toggle LED, delay BLINK_PERIOD_MS ms   */
    }
}
