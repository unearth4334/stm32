#include "app_freertos.h"
#include "app_blinky.h"
#include "osal/task.h"

#ifndef APP_BLINKY_TASK_STACK_WORDS
#define APP_BLINKY_TASK_STACK_WORDS 256U
#endif

#ifndef APP_BLINKY_TASK_PRIORITY
#define APP_BLINKY_TASK_PRIORITY 1U
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

void app_rtos_init(void)
{
    const osal_task_config_t cfg = {
        .name = "blinky",
        .stack_words = APP_BLINKY_TASK_STACK_WORDS,
        .priority = APP_BLINKY_TASK_PRIORITY
    };

    (void)osal_task_create(app_blinky_task, 0, &cfg);
}
