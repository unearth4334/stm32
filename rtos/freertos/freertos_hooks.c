#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "console/console.h"

void vApplicationMallocFailedHook(void)
{
    console_record_fault("malloc_failed", 0x4D414C4CU, 0U, 0U);
    console_log_panic("rtos", "malloc failed hook");

    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    console_record_fault("stack_overflow", 0x53544B4FU,
                         (uint32_t)(uintptr_t)xTask,
                         (uint32_t)(uintptr_t)pcTaskName);
    console_log_panic("rtos", "stack overflow task=%s", pcTaskName);

    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}
