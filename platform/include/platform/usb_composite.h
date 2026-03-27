/**
 * @file usb_composite.h
 * @brief Application-facing API for the CDC + HID Keyboard composite device.
 *
 * Provides the same CDC write / RX-callback interface as platform/usb_cdc.h
 * plus a new HID keyboard send function.  Include this instead of
 * platform/usb_cdc.h in projects that use the composite device.
 *
 * Dependency:
 *   platform_usb_composite_init() must be called once (after HAL_Init and
 *   SystemClock_Config) before any other function in this header.
 */

#ifndef PLATFORM_USB_COMPOSITE_H
#define PLATFORM_USB_COMPOSITE_H

#include <stdint.h>
#include "platform/usb_cdc.h"   /* re-use rx-callback typedef */

#ifdef __cplusplus
extern "C" {
#endif

/* ── Initialisation ──────────────────────────────────────────────────── */

/**
 * @brief Initialise the composite USB device (CDC-ACM + HID keyboard).
 *
 * Registers the CDC and HID sub-classes with the compositor, builds the
 * combined configuration descriptor, and starts USB enumeration.
 *
 * @return 0 on success, negative on error.
 */
int platform_usb_composite_init(void);

/* ── CDC interface ───────────────────────────────────────────────────── */

/**
 * @brief Transmit data over the CDC bulk endpoint.
 *
 * Blocks until the previous transfer completes or timeout_ms elapses.
 *
 * @param data       Pointer to transmit buffer.
 * @param len        Number of bytes to send.
 * @param timeout_ms Maximum time to wait for the endpoint to become free.
 * @return 0 on success, -1 on timeout or USB not ready.
 */
int platform_usb_composite_cdc_write(const uint8_t *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief Register a byte-at-a-time RX callback for CDC data received from host.
 *
 * @param cb   Callback invoked per received byte from ISR context.
 * @param ctx  Opaque context pointer forwarded to cb.
 * @return 0 on success.
 */
int platform_usb_composite_set_rx_callback(platform_usb_cdc_rx_cb_t cb, void *ctx);

/* ── HID keyboard interface ──────────────────────────────────────────── */

/**
 * @brief Send a single key-press HID report.
 *
 * Enqueues an 8-byte boot keyboard report on endpoint 0x83.
 * The host will see the key as held until key_release() is called.
 *
 * @param modifier  Modifier bitmask (HID_MOD_* constants from usbd_hid_kbd.h).
 * @param keycode   USB HID keyboard usage code (HID_KEY_* constants).
 * @return 0 on success, -1 if the endpoint is busy or USB not ready.
 */
int platform_usb_composite_hid_key_press(uint8_t modifier, uint8_t keycode);

/**
 * @brief Send an all-zero HID report (key release).
 *
 * Must be called after key_press() to release the key.
 * Typical sequence: key_press → HAL_Delay(10) → key_release.
 *
 * @return 0 on success, -1 if endpoint busy.
 */
int platform_usb_composite_hid_key_release(void);

/* ── IRQ handler (called from stm32xxxx_it.c) ───────────────────────── */

/** @brief Route USB interrupt to the correct HAL handler. */
void platform_usb_composite_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_USB_COMPOSITE_H */
