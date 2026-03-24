#ifndef OSAL_TASK_H
#define OSAL_TASK_H

#include <stdint.h>

typedef void (*osal_task_fn_t)(void *arg);

typedef struct
{
    const char *name;
    uint16_t stack_words;
    uint8_t priority;
} osal_task_config_t;

enum
{
    OSAL_OK = 0,
    OSAL_ERR_INVALID_ARG = -1,
    OSAL_ERR_NOT_SUPPORTED = -2,
    OSAL_ERR_NO_RESOURCES = -3
};

int osal_task_create(osal_task_fn_t fn, void *arg, const osal_task_config_t *cfg);
void osal_scheduler_start(void);

#endif /* OSAL_TASK_H */
