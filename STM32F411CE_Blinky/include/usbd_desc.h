#ifndef USBD_DESC_H
#define USBD_DESC_H

#include "usbd_def.h"

#define USBD_VID                     0x0483U
#define USBD_PID                     0x5740U
#define USBD_LANGID_STRING           0x0409U
#define USBD_MANUFACTURER_STRING     "STMicroelectronics"
#define USBD_PRODUCT_STRING_FS       "STM32 STS4x Console"
#define USBD_CONFIGURATION_STRING_FS "CDC Config"
#define USBD_INTERFACE_STRING_FS     "CDC Interface"

extern USBD_DescriptorsTypeDef FS_Desc;

#endif
