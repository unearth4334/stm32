/**
 * @file usbd_cdc_if_composite.c
 * @brief USB CDC application interface for the composite CDC+HID device.
 *
 * This file replaces the single-class usbd_cdc_if.c for composite builds.
 * The sole functional difference: CDC_Receive_FS() routes receive data to
 * platform_usb_composite_on_rx_data() instead of platform_usb_cdc_on_rx_data(),
 * because the registered RX callback is managed by the composite layer.
 *
 * Compatible with both STM32F4 and STM32L0 composite library targets.
 */

#include "usbd_cdc_if.h"

#include <string.h>

#include "usbd_def.h"
#include "usbd_core.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

/* Forward declaration — defined in platform_usb_composite.c (F4 or L0) */
void platform_usb_composite_on_rx_data(const uint8_t *buf, uint32_t len);

#define APP_RX_DATA_SIZE  512U
#define APP_TX_DATA_SIZE  512U

static uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
static uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

static int8_t CDC_Init_FS(void)
{
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0U);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
    return (int8_t)USBD_OK;
}

static int8_t CDC_DeInit_FS(void)
{
    return (int8_t)USBD_OK;
}

static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    (void)cmd;
    (void)pbuf;
    (void)length;
    return (int8_t)USBD_OK;
}

static int8_t CDC_Receive_FS(uint8_t *buf, uint32_t *len)
{
    /* Route to composite handler instead of single-class CDC handler */
    platform_usb_composite_on_rx_data(buf, *len);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, buf);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return (int8_t)USBD_OK;
}

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {
    CDC_Init_FS,
    CDC_DeInit_FS,
    CDC_Control_FS,
    CDC_Receive_FS
};

uint8_t CDC_Transmit_FS(uint8_t *buf, uint16_t len)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
    if (hcdc == NULL) {
        return (uint8_t)USBD_FAIL;
    }

    if (hcdc->TxState != 0U) {
        return (uint8_t)USBD_BUSY;
    }

    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, buf, len);
    return USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}
