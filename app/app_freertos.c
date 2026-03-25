#include "app_freertos.h"
#include "app_blinky.h"
#include "osal/task.h"
#include "osal/delay.h"
#include "console/console.h"

#ifndef APP_BLINKY_TASK_STACK_WORDS
#define APP_BLINKY_TASK_STACK_WORDS 256U
#endif

#ifndef APP_BLINKY_TASK_PRIORITY
#define APP_BLINKY_TASK_PRIORITY 1U
#endif

#ifndef APP_CONSOLE_TASK_STACK_WORDS
#define APP_CONSOLE_TASK_STACK_WORDS 256U
#endif

#ifndef APP_CONSOLE_TASK_PRIORITY
#define APP_CONSOLE_TASK_PRIORITY 1U
#endif

#ifndef APP_CONSOLE_POLL_PERIOD_MS
#define APP_CONSOLE_POLL_PERIOD_MS 10U
#endif

static void app_blinky_task(void *arg)
{
    (void)arg;

    app_blinky_init();

    while (1)
    {
        app_blinky_run();
    }
}

static void app_console_task(void *arg)
{
    (void)arg;

    while (1)
    {
        /* console_poll() blocks internally on the RX stream buffer,
         * so no artificial delay is needed here. */
        console_poll();
    }
}

void app_rtos_init(void)
{
    const osal_task_config_t cfg = {
        .name = "blinky",
        .stack_words = APP_BLINKY_TASK_STACK_WORDS,
        .priority = APP_BLINKY_TASK_PRIORITY
    };

    const osal_task_config_t console_cfg = {
        .name = "console",
        .stack_words = APP_CONSOLE_TASK_STACK_WORDS,
        .priority = APP_CONSOLE_TASK_PRIORITY
    };

    (void)osal_task_create(app_blinky_task, 0, &cfg);
    (void)osal_task_create(app_console_task, 0, &console_cfg);
}
