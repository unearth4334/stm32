#include "usb_device.h"

#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_core.h"
#include "usbd_desc.h"

USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_DEVICE_Init(void)
{
    (void)USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
    (void)USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
    (void)USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);
    (void)USBD_Start(&hUsbDeviceFS);
}
