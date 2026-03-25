#include "platform/usb_cdc.h"

#include <stddef.h>

#include "board.h"
#include "stm32f4xx_hal.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_desc.h"
#include "usbd_cdc_if.h"

USBD_HandleTypeDef hUsbDeviceFS;
PCD_HandleTypeDef hpcd_USB_OTG_FS;

static uint8_t s_usb_ready;
static platform_usb_cdc_rx_cb_t s_rx_cb;
static void *s_rx_ctx;

int platform_usb_cdc_init(void)
{
    if (s_usb_ready) {
        return 0;
    }

    if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
        return -1;
    }

    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK) {
        return -1;
    }

    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK) {
        return -1;
    }

    if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
        return -1;
    }

    s_usb_ready = 1U;
    return 0;
}

int platform_usb_cdc_write(const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    uint32_t t0;

    if ((data == NULL) || (len == 0U) || (s_usb_ready == 0U)) {
        return -1;
    }

    t0 = HAL_GetTick();
    while (CDC_Transmit_FS((uint8_t *)data, len) == USBD_BUSY) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            return -1;
        }
    }

    return 0;
}

int platform_usb_cdc_set_rx_callback(platform_usb_cdc_rx_cb_t cb, void *ctx)
{
    s_rx_cb = cb;
    s_rx_ctx = ctx;
    return 0;
}

void platform_usb_cdc_on_rx_data(const uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if ((buf == NULL) || (s_rx_cb == NULL)) {
        return;
    }

    for (i = 0U; i < len; i++) {
        s_rx_cb(buf[i], s_rx_ctx);
    }
}

void platform_usb_cdc_irq_handler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
