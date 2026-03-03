#include "usb_console_port.h"

#include "usbd_cdc_if.h"

__attribute__((weak)) int32_t UsbConsole_ReadByte(uint8_t *byte_out)
{
    return UsbCdc_ReadByte(byte_out);
}

__attribute__((weak)) int32_t UsbConsole_Write(const uint8_t *data, uint16_t length)
{
    return UsbCdc_Write(data, length);
}
