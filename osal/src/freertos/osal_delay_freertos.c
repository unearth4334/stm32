#include "osal/delay.h"
#include "platform/clock.h"

#include "FreeRTOS.h"
#include "task.h"

void osal_delay_ms(uint32_t ms)
{
    if (ms == 0U)
    {
        return;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        platform_delay_ms(ms);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(ms));
}
