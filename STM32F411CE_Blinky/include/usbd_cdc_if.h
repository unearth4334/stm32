#ifndef USBD_CDC_IF_H
#define USBD_CDC_IF_H

#include <stdint.h>

#include "usbd_cdc.h"

extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;

int32_t UsbCdc_ReadByte(uint8_t *byte_out);
int32_t UsbCdc_Write(const uint8_t *data, uint16_t length);
uint8_t UsbCdc_IsConfigured(void);

#endif
