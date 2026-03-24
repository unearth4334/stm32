#include "osal/task.h"

#include "FreeRTOS.h"
#include "task.h"

int osal_task_create(osal_task_fn_t fn, void *arg, const osal_task_config_t *cfg)
{
    BaseType_t status;

    if ((fn == 0) || (cfg == 0) || (cfg->name == 0) || (cfg->stack_words == 0U))
    {
        return OSAL_ERR_INVALID_ARG;
    }

    status = xTaskCreate(fn,
                         cfg->name,
                         (configSTACK_DEPTH_TYPE)cfg->stack_words,
                         arg,
                         (UBaseType_t)cfg->priority,
                         0);

    if (status != pdPASS)
    {
        return OSAL_ERR_NO_RESOURCES;
    }

    return OSAL_OK;
}

void osal_scheduler_start(void)
{
    vTaskStartScheduler();

    /* Should never return; if it does, trap for debug visibility. */
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}
