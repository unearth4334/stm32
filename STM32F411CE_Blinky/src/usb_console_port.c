#include "usb_console_port.h"

__attribute__((weak)) int32_t UsbConsole_ReadByte(uint8_t *byte_out)
{
    (void)byte_out;
    return 0;
}

__attribute__((weak)) int32_t UsbConsole_Write(const uint8_t *data, uint16_t length)
{
    (void)data;
    (void)length;
    return 0;
}
