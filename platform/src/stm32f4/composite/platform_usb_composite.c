/**
 * @file platform_usb_composite.c
 * @brief STM32F4 platform implementation of the composite USB API.
 *
 * Wires together:
 *   - USBD middleware (usbd_core)
 *   - CDC class driver (usbd_cdc)
 *   - HID keyboard class driver (usbd_hid_kbd)
 *   - Composite dispatcher (usbd_composite)
 *   - Combined descriptor (usbd_cdc_hid_desc)
 *
 * Sub-class index assignments:
 *   Index 0 → CDC   (interfaces 0-1, EP1_IN 0x81, EP1_OUT 0x01, EP2_IN 0x82)
 *   Index 1 → HID   (interface  2,   EP3_IN 0x83)
 */

#include "platform/usb_composite.h"

#include "stm32f4xx_hal.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"        /* USBD_Interface_fops_FS, CDC_Transmit_FS */
#include "usbd_composite.h"
#include "usbd_cdc_hid_desc.h"
#include "usbd_hid_kbd.h"

/* ── Module state ────────────────────────────────────────────────────── */
USBD_HandleTypeDef  hUsbDeviceFS;
PCD_HandleTypeDef   hpcd_USB_OTG_FS;

static uint8_t                 s_ready;
static platform_usb_cdc_rx_cb_t s_rx_cb;
static void                   *s_rx_ctx;

/* ── CDC endpoint list (index 0) ─────────────────────────────────────── */
static const usbd_sub_class_t s_cdc_entry = {
    .cls        = &USBD_CDC,
    .user_data  = &USBD_Interface_fops_FS,
    .first_itf  = 0U,
    .itf_count  = 2U,
    .ep_in      = { 0x81U, 0x82U, 0x00U, 0x00U },
    .ep_out     = { 0x01U, 0x00U, 0x00U, 0x00U },
};

/* ── HID endpoint list (index 1) ──────────────────────────────────────── */
static const usbd_sub_class_t s_hid_entry = {
    .cls        = &USBD_HID_Keyboard,
    .user_data  = NULL,
    .first_itf  = 2U,
    .itf_count  = 1U,
    .ep_in      = { 0x83U, 0x00U, 0x00U, 0x00U },
    .ep_out     = { 0x00U, 0x00U, 0x00U, 0x00U },
};

/* ── Init ────────────────────────────────────────────────────────────── */

int platform_usb_composite_init(void)
{
    if (s_ready) {
        return 0;
    }

    usbd_composite_register(&s_cdc_entry);
    usbd_composite_register(&s_hid_entry);

    if (USBD_Init(&hUsbDeviceFS, &CDC_HID_Desc, DEVICE_FS) != USBD_OK) {
        return -1;
    }

    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_Composite) != USBD_OK) {
        return -1;
    }

    /* pUserData must point to CDC ops before USBD_Start triggers Init() */
    hUsbDeviceFS.pUserData = &USBD_Interface_fops_FS;

    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK) {
        return -1;
    }

    if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
        return -1;
    }

    s_ready = 1U;
    return 0;
}

/* ── CDC write ───────────────────────────────────────────────────────── */

int platform_usb_composite_cdc_write(const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    uint32_t t0;

    if ((data == NULL) || (len == 0U) || (s_ready == 0U)) {
        return -1;
    }

    /* Swap pClassData to CDC state before calling ST CDC transmit */
    void *saved = hUsbDeviceFS.pClassData;
    hUsbDeviceFS.pClassData = usbd_composite_get_state(0U); /* CDC */

    t0 = HAL_GetTick();
    while (CDC_Transmit_FS((uint8_t *)data, len) == USBD_BUSY) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            hUsbDeviceFS.pClassData = saved;
            return -1;
        }
    }

    hUsbDeviceFS.pClassData = saved;
    return 0;
}

/* ── CDC RX callback ──────────────────────────────────────────────────── */

int platform_usb_composite_set_rx_callback(platform_usb_cdc_rx_cb_t cb, void *ctx)
{
    s_rx_cb  = cb;
    s_rx_ctx = ctx;
    return 0;
}

/**
 * @brief Called from usbd_cdc_if.c CDC_Receive_FS() when data arrives.
 *        Forwards to the registered byte-at-a-time callback.
 */
void platform_usb_composite_on_rx_data(const uint8_t *buf, uint32_t len)
{
    if ((buf == NULL) || (s_rx_cb == NULL)) {
        return;
    }
    for (uint32_t i = 0U; i < len; i++) {
        s_rx_cb(buf[i], s_rx_ctx);
    }
}

/* ── HID keyboard ─────────────────────────────────────────────────────── */

int platform_usb_composite_hid_key_press(uint8_t modifier, uint8_t keycode)
{
    if (s_ready == 0U) {
        return -1;
    }

    uint8_t report[8] = { modifier, 0x00U, keycode, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U };

    void *saved = hUsbDeviceFS.pClassData;
    hUsbDeviceFS.pClassData = usbd_composite_get_state(1U); /* HID */
    uint8_t ret = USBD_HID_KBD_SendReport(&hUsbDeviceFS, report, sizeof(report));
    hUsbDeviceFS.pClassData = saved;

    return (ret == USBD_OK) ? 0 : -1;
}

int platform_usb_composite_hid_key_release(void)
{
    if (s_ready == 0U) {
        return -1;
    }

    uint8_t report[8] = { 0U };

    void *saved = hUsbDeviceFS.pClassData;
    hUsbDeviceFS.pClassData = usbd_composite_get_state(1U); /* HID */
    uint8_t ret = USBD_HID_KBD_SendReport(&hUsbDeviceFS, report, sizeof(report));
    hUsbDeviceFS.pClassData = saved;

    return (ret == USBD_OK) ? 0 : -1;
}

/* ── IRQ routing ──────────────────────────────────────────────────────── */

void platform_usb_composite_irq_handler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
