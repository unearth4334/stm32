#include "osal/delay.h"
#include "platform/clock.h"

void osal_delay_ms(uint32_t ms)
{
    platform_delay_ms(ms);
}
