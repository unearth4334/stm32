#ifndef USBD_CONF_H
#define USBD_CONF_H

#include <string.h>

#include "stm32f4xx.h"

#define USBD_MAX_NUM_INTERFACES     1U
#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       0x100U
#define USBD_SELF_POWERED           1U
#define USBD_DEBUG_LEVEL            0U

#define DEVICE_FS                   0U

#define USBD_malloc                 USBD_static_malloc
#define USBD_free                   USBD_static_free
#define USBD_memset                 memset
#define USBD_memcpy                 memcpy
#define USBD_Delay                  USBD_LL_Delay

void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);
void USBD_LL_Delay(uint32_t Delay);

#endif /* USBD_CONF_H */
