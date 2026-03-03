#ifndef USB_CONSOLE_PORT_H
#define USB_CONSOLE_PORT_H

#include <stdint.h>

int32_t UsbConsole_ReadByte(uint8_t *byte_out);
int32_t UsbConsole_Write(const uint8_t *data, uint16_t length);

#endif
