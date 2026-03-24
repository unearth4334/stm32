#include "osal/task.h"

static osal_task_fn_t g_entry;
static void *g_arg;

int osal_task_create(osal_task_fn_t fn, void *arg, const osal_task_config_t *cfg)
{
    if ((fn == 0) || (cfg == 0))
    {
        return OSAL_ERR_INVALID_ARG;
    }

    if (g_entry != 0)
    {
        return OSAL_ERR_NO_RESOURCES;
    }

    g_entry = fn;
    g_arg = arg;
    return OSAL_OK;
}

void osal_scheduler_start(void)
{
    if (g_entry != 0)
    {
        g_entry(g_arg);
    }

    while (1)
    {
        /* Bare-metal fallback: scheduler is not available yet. */
    }
}
