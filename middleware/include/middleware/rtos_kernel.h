#ifndef MIDDLEWARE_RTOS_KERNEL_H
#define MIDDLEWARE_RTOS_KERNEL_H

#include <stdint.h>

typedef void (*rtos_task_fn_t)(void *context);

typedef struct {
    const char *name;
    rtos_task_fn_t entry;
    void *context;
    uint16_t stack_words;
    uint32_t priority;
} rtos_task_config_t;

void rtos_kernel_init(void);
int rtos_task_create(const rtos_task_config_t *task_config);
void rtos_kernel_start(void);
void rtos_delay_ms(uint32_t delay_ms);

#endif
