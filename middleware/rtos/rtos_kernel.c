#include "middleware/rtos_kernel.h"

#if defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

void rtos_kernel_init(void)
{
}

int rtos_task_create(const rtos_task_config_t *task_config)
{
    if (task_config == 0 || task_config->entry == 0 || task_config->name == 0) {
        return -1;
    }

#if defined(USE_FREERTOS)
    BaseType_t result = xTaskCreate(
        task_config->entry,
        task_config->name,
        task_config->stack_words,
        task_config->context,
        task_config->priority,
        0
    );

    return (result == pdPASS) ? 0 : -1;
#else
    return 0;
#endif
}

void rtos_kernel_start(void)
{
#if defined(USE_FREERTOS)
    vTaskStartScheduler();
#endif
}

void rtos_delay_ms(uint32_t delay_ms)
{
#if defined(USE_FREERTOS)
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
#else
    (void)delay_ms;
#endif
}
