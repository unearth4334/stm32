#ifndef OSAL_DELAY_H
#define OSAL_DELAY_H

#include <stdint.h>

/* OS-agnostic millisecond delay API. */
void osal_delay_ms(uint32_t ms);

#endif /* OSAL_DELAY_H */
