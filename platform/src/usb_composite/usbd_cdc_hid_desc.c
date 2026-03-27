/**
 * @file usbd_cdc_hid_desc.c
 * @brief Combined CDC-ACM + HID Keyboard USB descriptor tables.
 *
 * Configuration descriptor layout (100 bytes):
 *
 *   Configuration descriptor      9 B
 *   IAD  (interfaces 0-1, CDC)    8 B
 *   Interface 0: CDC-Control       9 B
 *     CDC Header FD                5 B
 *     CDC Call-Management FD       5 B
 *     CDC ACM FD                   4 B
 *     CDC Union FD                 5 B
 *     EP 0x82: interrupt IN 8 B   7 B   [CDC_CMD_EP]
 *   Interface 1: CDC-Data          9 B
 *     EP 0x01: bulk OUT 64 B      7 B   [CDC_OUT_EP]
 *     EP 0x81: bulk IN  64 B      7 B   [CDC_IN_EP]
 *   Interface 2: HID Keyboard      9 B
 *     HID descriptor               9 B
 *     EP 0x83: interrupt IN 8 B   7 B   [HID_EPIN_ADDR]
 *
 *   Total = 9+8+9+5+5+4+5+7+9+7+7+9+9+7 = 100 B
 */

#include "usbd_cdc_hid_desc.h"
#include "usbd_hid_kbd.h"

#include "usbd_core.h"

/* ── VID / PID / strings ──────────────────────────────────────────────── */
#define USBD_VID                    0x0483U   /* STMicroelectronics    */
#define USBD_PID                    0x5711U   /* CDC + HID composite   */
#define USBD_LANGID                 0x0409U   /* English (US)          */
#define USBD_MANUFACTURER_STR       "Redlen"
#define USBD_PRODUCT_STR            "Composite CDC+HID"
#define USBD_SERIAL_STR             "000000000002"
#define USBD_CONFIGURATION_STR      "CDC+HID Config"
#define USBD_INTERFACE_STR          "CDC+HID Interface"

/* ── CDC endpoint numbers (match usbd_cdc.h defaults) ────────────────────  */
#define CDC_IN_EP                   0x81U
#define CDC_OUT_EP                  0x01U
#define CDC_CMD_EP                  0x82U
#define CDC_CMD_PACKET_SIZE         8U
#define CDC_DATA_FS_MAX_PACKET_SIZE 64U

/* ── Aggregate descriptor ─────────────────────────────────────────────── */
static uint8_t s_config_desc[USBD_CDC_HID_CONFIG_DESC_SIZE] = {
    /* --- Configuration descriptor (9 bytes) --- */
    0x09U,                          /* bLength                      */
    USB_DESC_TYPE_CONFIGURATION,    /* bDescriptorType (2)          */
    LOBYTE(USBD_CDC_HID_CONFIG_DESC_SIZE),
    HIBYTE(USBD_CDC_HID_CONFIG_DESC_SIZE),
    0x03U,                          /* bNumInterfaces (CDC x2 + HID)*/
    0x01U,                          /* bConfigurationValue          */
    0x00U,                          /* iConfiguration               */
    0xC0U,                          /* bmAttributes (self-powered)  */
    0x32U,                          /* bMaxPower 100 mA             */

    /* --- IAD: groups CDC-Control + CDC-Data (8 bytes) --- */
    0x08U,                          /* bLength                      */
    0x0BU,                          /* bDescriptorType (IAD = 0x0B) */
    0x00U,                          /* bFirstInterface (0)          */
    0x02U,                          /* bInterfaceCount (2)          */
    0x02U,                          /* bFunctionClass (CDC)         */
    0x02U,                          /* bFunctionSubClass (ACM)      */
    0x01U,                          /* bFunctionProtocol (AT)       */
    0x00U,                          /* iFunction                    */

    /* --- Interface 0: CDC Communication (9 bytes) --- */
    0x09U,                          /* bLength                      */
    USB_DESC_TYPE_INTERFACE,        /* bDescriptorType (4)          */
    0x00U,                          /* bInterfaceNumber (0)         */
    0x00U,                          /* bAlternateSetting            */
    0x01U,                          /* bNumEndpoints                */
    0x02U,                          /* bInterfaceClass (CDC)        */
    0x02U,                          /* bInterfaceSubClass (ACM)     */
    0x01U,                          /* bInterfaceProtocol (AT cmd)  */
    0x00U,                          /* iInterface                   */

    /* CDC Header Functional Descriptor (5 bytes) */
    0x05U, 0x24U, 0x00U, 0x10U, 0x01U,

    /* CDC Call Management Functional Descriptor (5 bytes) */
    0x05U, 0x24U, 0x01U, 0x00U, 0x01U,

    /* CDC ACM Functional Descriptor (4 bytes) */
    0x04U, 0x24U, 0x02U, 0x02U,

    /* CDC Union Functional Descriptor (5 bytes) */
    0x05U, 0x24U, 0x06U,
    0x00U,                          /* bControlInterface (0)        */
    0x01U,                          /* bSubordinateInterface0 (1)   */

    /* Endpoint 0x82: CDC notify, interrupt IN (7 bytes) */
    0x07U,                          /* bLength                      */
    USB_DESC_TYPE_ENDPOINT,         /* bDescriptorType (5)          */
    CDC_CMD_EP,                     /* bEndpointAddress (0x82)      */
    0x03U,                          /* bmAttributes (interrupt)     */
    LOBYTE(CDC_CMD_PACKET_SIZE),
    HIBYTE(CDC_CMD_PACKET_SIZE),
    0x10U,                          /* bInterval 16 ms              */

    /* --- Interface 1: CDC Data (9 bytes) --- */
    0x09U,
    USB_DESC_TYPE_INTERFACE,
    0x01U,                          /* bInterfaceNumber (1)         */
    0x00U,                          /* bAlternateSetting            */
    0x02U,                          /* bNumEndpoints                */
    0x0AU,                          /* bInterfaceClass (CDC-Data)   */
    0x00U,                          /* bInterfaceSubClass           */
    0x00U,                          /* bInterfaceProtocol           */
    0x00U,                          /* iInterface                   */

    /* Endpoint 0x01: CDC data, bulk OUT (7 bytes) */
    0x07U,
    USB_DESC_TYPE_ENDPOINT,
    CDC_OUT_EP,                     /* bEndpointAddress (0x01)      */
    0x02U,                          /* bmAttributes (bulk)          */
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
    HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
    0x00U,                          /* bInterval (ignored for bulk) */

    /* Endpoint 0x81: CDC data, bulk IN (7 bytes) */
    0x07U,
    USB_DESC_TYPE_ENDPOINT,
    CDC_IN_EP,                      /* bEndpointAddress (0x81)      */
    0x02U,                          /* bmAttributes (bulk)          */
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
    HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
    0x00U,

    /* --- Interface 2: HID Keyboard (9 bytes) --- */
    0x09U,
    USB_DESC_TYPE_INTERFACE,
    0x02U,                          /* bInterfaceNumber (2)         */
    0x00U,                          /* bAlternateSetting            */
    0x01U,                          /* bNumEndpoints                */
    0x03U,                          /* bInterfaceClass (HID)        */
    0x01U,                          /* bInterfaceSubClass (Boot)    */
    0x01U,                          /* bInterfaceProtocol (Keyboard)*/
    0x00U,                          /* iInterface                   */

    /* HID Descriptor (9 bytes) */
    0x09U,                          /* bLength                      */
    0x21U,                          /* bDescriptorType (HID = 0x21) */
    0x11U, 0x01U,                   /* bcdHID 1.11                  */
    0x00U,                          /* bCountryCode                 */
    0x01U,                          /* bNumDescriptors              */
    0x22U,                          /* bClassDescriptorType (Report)*/
    0x2DU, 0x00U,                   /* wDescriptorLength (45 bytes) */

    /* Endpoint 0x83: HID, interrupt IN (7 bytes) */
    0x07U,
    USB_DESC_TYPE_ENDPOINT,
    HID_EPIN_ADDR,                  /* bEndpointAddress (0x83)      */
    0x03U,                          /* bmAttributes (interrupt)     */
    LOBYTE(HID_EPIN_SIZE),
    HIBYTE(HID_EPIN_SIZE),
    HID_FS_BINTERVAL                /* bInterval 10 ms              */
};

/* ── Device descriptor ───────────────────────────────────────────────── */
/* Device class MISC + IAD protocol required when IAD is in config desc.  */
static uint8_t s_device_desc[USB_LEN_DEV_DESC] = {
    0x12U,                          /* bLength                      */
    USB_DESC_TYPE_DEVICE,           /* bDescriptorType (1)          */
    0x00U, 0x02U,                   /* bcdUSB 2.00                  */
    0xEFU,                          /* bDeviceClass (Misc)          */
    0x02U,                          /* bDeviceSubClass (Common)     */
    0x01U,                          /* bDeviceProtocol (IAD)        */
    USB_MAX_EP0_SIZE,               /* bMaxPacketSize0 (64)         */
    LOBYTE(USBD_VID),
    HIBYTE(USBD_VID),
    LOBYTE(USBD_PID),
    HIBYTE(USBD_PID),
    0x00U, 0x02U,                   /* bcdDevice 2.00               */
    USBD_IDX_MFC_STR,
    USBD_IDX_PRODUCT_STR,
    USBD_IDX_SERIAL_STR,
    USBD_MAX_NUM_CONFIGURATION
};

static uint8_t s_lang_id_desc[USB_LEN_LANGID_STR_DESC] = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID),
    HIBYTE(USBD_LANGID)
};

static uint8_t s_str_desc[USBD_MAX_STR_DESC_SIZ];

/* ── usbd_cdc_hid_get_fs_config_desc (called by composite vtable) ─────── */

uint8_t *usbd_cdc_hid_get_fs_config_desc(uint16_t *length)
{
    *length = USBD_CDC_HID_CONFIG_DESC_SIZE;
    return s_config_desc;
}

/* ── USBD_DescriptorsTypeDef callbacks ────────────────────────────────── */

static uint8_t *CDC_HID_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(s_device_desc);
    return s_device_desc;
}

static uint8_t *CDC_HID_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(s_lang_id_desc);
    return s_lang_id_desc;
}

static uint8_t *CDC_HID_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STR, s_str_desc, length);
    return s_str_desc;
}

static uint8_t *CDC_HID_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_PRODUCT_STR, s_str_desc, length);
    return s_str_desc;
}

static uint8_t *CDC_HID_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_SERIAL_STR, s_str_desc, length);
    return s_str_desc;
}

static uint8_t *CDC_HID_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_STR, s_str_desc, length);
    return s_str_desc;
}

static uint8_t *CDC_HID_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_INTERFACE_STR, s_str_desc, length);
    return s_str_desc;
}

USBD_DescriptorsTypeDef CDC_HID_Desc = {
    CDC_HID_DeviceDescriptor,
    CDC_HID_LangIDStrDescriptor,
    CDC_HID_ManufacturerStrDescriptor,
    CDC_HID_ProductStrDescriptor,
    CDC_HID_SerialStrDescriptor,
    CDC_HID_ConfigStrDescriptor,
    CDC_HID_InterfaceStrDescriptor
};
