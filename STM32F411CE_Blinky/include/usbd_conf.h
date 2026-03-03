#ifndef USBD_CONF_H
#define USBD_CONF_H

#include <stdint.h>

#include "stm32f4xx.h"

#define USBD_MAX_NUM_INTERFACES            1U
#define USBD_MAX_NUM_CONFIGURATION         1U
#define USBD_MAX_STR_DESC_SIZ              0x100U
#define USBD_SUPPORT_USER_STRING           0U
#define USBD_SELF_POWERED                  1U
#define USBD_DEBUG_LEVEL                   0U

#define DEVICE_FS                          0U

#define USBD_CDC_INTERVAL                  0x10U
#define USBD_CDC_CMD_PACKET_SIZE           8U
#define USBD_CDC_DATA_MAX_PACKET_SIZE      64U

#define USBD_malloc                        USBD_static_malloc
#define USBD_free                          USBD_static_free
#define USBD_memset                        memset
#define USBD_memcpy                        memcpy
#define USBD_Delay                         USBD_LL_Delay

#include <string.h>

#if (USBD_DEBUG_LEVEL > 0U)
#include <stdio.h>
#define USBD_UsrLog(...)   do { printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBD_UsrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 1U)
#include <stdio.h>
#define USBD_ErrLog(...)   do { printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBD_ErrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 2U)
#include <stdio.h>
#define USBD_DbgLog(...)   do { printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBD_DbgLog(...)
#endif

void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);
void USBD_LL_Delay(uint32_t delay);

#endif
