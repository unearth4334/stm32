/**
 * @file main.c
 * @brief Entry point — blackpill_f411ce_blinky project.
 *
 * Dependency chain (read bottom-up):
 *   vendor (STM32CubeF4 / CMSIS)
 *     └─ board   (boards/blackpill_f411ce)
 *         └─ platform (platform/stm32f4/hal)
 *             └─ drivers (drivers/led)
 *                 └─ services (services/led_service)
 *                     └─ app (app/app_blinky)  ← main calls this
 *
 * main() itself is kept minimal: initialise, then loop.
 */

#include "platform/clock.h"
#include "board.h"
#include "app_blinky.h"
#include "app_freertos.h"
#include "osal/task.h"

/* Defined in platform/src/stm32f4/common/platform_init.c */
extern void platform_init(void);

int main(void)
{
    platform_init();   /* HAL_Init + SystemClock_Config              */
    board_init();      /* Enable GPIO clocks, configure board hw     */

#if defined(USE_FREERTOS)
    app_rtos_init();
    osal_scheduler_start();
#else
    app_blinky_init(); /* Init services → drivers → platform GPIO   */

    while (1)
    {
        app_blinky_run(); /* Toggle LED, delay BLINK_PERIOD_MS ms   */
    }
#endif
}
