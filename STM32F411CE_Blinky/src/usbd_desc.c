#include "usbd_desc.h"

#include "stm32f4xx.h"
#include "usbd_core.h"

#define USBD_MAX_STR_DESC_SIZ 0x100U

#define USB_SIZ_STRING_SERIAL 0x1AU

static uint8_t USBD_FS_DeviceDesc[USB_LEN_DEV_DESC] = {
    0x12,                       /* bLength */
    USB_DESC_TYPE_DEVICE,       /* bDescriptorType */
    0x00, 0x02,                 /* bcdUSB */
    0x02,                       /* bDeviceClass */
    0x02,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    USB_MAX_EP0_SIZE,           /* bMaxPacketSize */
    LOBYTE(USBD_VID), HIBYTE(USBD_VID),
    LOBYTE(USBD_PID), HIBYTE(USBD_PID),
    0x00, 0x02,                 /* bcdDevice rel. 2.00 */
    USBD_IDX_MFC_STR,
    USBD_IDX_PRODUCT_STR,
    USBD_IDX_SERIAL_STR,
    USBD_MAX_NUM_CONFIGURATION
};

static uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING), HIBYTE(USBD_LANGID_STRING),
};

static uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ];

static void Get_SerialNum(void);
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len);

uint8_t *USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(USBD_FS_DeviceDesc);
    return USBD_FS_DeviceDesc;
}

uint8_t *USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(USBD_LangIDDesc);
    return USBD_LangIDDesc;
}

uint8_t *USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

uint8_t *USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    if (speed == USBD_SPEED_HIGH)
    {
        USBD_GetString((uint8_t *)USBD_PRODUCT_STRING_FS, USBD_StrDesc, length);
        return USBD_StrDesc;
    }

    USBD_GetString((uint8_t *)USBD_PRODUCT_STRING_FS, USBD_StrDesc, length);
    return USBD_StrDesc;
}

uint8_t *USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = USB_SIZ_STRING_SERIAL;
    Get_SerialNum();
    return USBD_StrDesc;
}

uint8_t *USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING_FS, USBD_StrDesc, length);
    return USBD_StrDesc;
}

uint8_t *USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_INTERFACE_STRING_FS, USBD_StrDesc, length);
    return USBD_StrDesc;
}

USBD_DescriptorsTypeDef FS_Desc = {
    USBD_FS_DeviceDescriptor,
    USBD_FS_LangIDStrDescriptor,
    USBD_FS_ManufacturerStrDescriptor,
    USBD_FS_ProductStrDescriptor,
    USBD_FS_SerialStrDescriptor,
    USBD_FS_ConfigStrDescriptor,
    USBD_FS_InterfaceStrDescriptor,
};

static void Get_SerialNum(void)
{
    uint32_t deviceserial0 = *(uint32_t *)UID_BASE;
    uint32_t deviceserial1 = *(uint32_t *)(UID_BASE + 4U);
    uint32_t deviceserial2 = *(uint32_t *)(UID_BASE + 8U);

    deviceserial0 += deviceserial2;

    if (deviceserial0 != 0U)
    {
        IntToUnicode(deviceserial0, &USBD_StrDesc[2], 8U);
        IntToUnicode(deviceserial1, &USBD_StrDesc[18], 4U);
    }

    USBD_StrDesc[0] = USB_SIZ_STRING_SERIAL;
    USBD_StrDesc[1] = USB_DESC_TYPE_STRING;
}

static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    uint8_t index;

    for (index = 0U; index < len; index++)
    {
        if (((value >> 28U)) < 0xAU)
        {
            pbuf[2U * index] = (uint8_t)((value >> 28U) + '0');
        }
        else
        {
            pbuf[2U * index] = (uint8_t)((value >> 28U) + 'A' - 10U);
        }

        value <<= 4U;
        pbuf[(2U * index) + 1U] = 0U;
    }
}
