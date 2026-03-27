/**
 * @file usbd_composite.h
 * @brief Composite USB device dispatcher.
 *
 * Registers multiple ST USB class drivers (e.g. CDC + HID keyboard) under
 * one USBD_ClassTypeDef so that ST middleware's USBD_RegisterClass() can
 * manage them as a single composite device.
 *
 * Usage:
 *   1. Build the combined configuration descriptor (see usbd_cdc_hid_desc.h).
 *   2. Call usbd_composite_register() once per sub-class, assigning each its
 *      starting interface number and IN/OUT endpoint list.
 *   3. Pass &USBD_Composite to USBD_RegisterClass().
 *
 * Dispatch strategy:
 *   - Init / DeInit    : broadcast to all sub-classes.
 *   - Setup            : routed by interface number (req->wIndex & 0xFF).
 *   - EP0_RxReady      : routed to the sub-class that last received a Setup.
 *   - DataIn / DataOut : routed by endpoint address.
 *
 * pClassData management:
 *   Each sub-class's Init() allocates its own state and stores it in
 *   pdev->pClassData.  The compositor saves each pointer and swaps in the
 *   correct one before every sub-class callback so that ST CDC/HID code that
 *   reads pClassData continues to work without modification.
 */

#ifndef USBD_COMPOSITE_H
#define USBD_COMPOSITE_H

#include "usbd_def.h"

/* Maximum number of simultaneously registered sub-classes. -------------- */
#define USBD_COMPOSITE_MAX_CLASSES  4U

/* Maximum number of IN or OUT endpoints per sub-class. ------------------- */
#define USBD_COMPOSITE_MAX_EPS      4U

/**
 * @brief Sub-class registration descriptor.
 *
 * Populate one of these for every USB class you want in the composite device
 * and pass it to usbd_composite_register() before composite_init is invoked.
 */
typedef struct {
    const USBD_ClassTypeDef *cls;               /**< ST class driver vtable          */
    void                    *user_data;          /**< Value pre-loaded into pUserData  */
    uint8_t                  first_itf;          /**< First interface number           */
    uint8_t                  itf_count;          /**< Number of interfaces             */
    uint8_t                  ep_in[USBD_COMPOSITE_MAX_EPS]; /**< IN EP addrs, 0-term  */
    uint8_t                  ep_out[USBD_COMPOSITE_MAX_EPS];/**< OUT EP addrs, 0-term */
} usbd_sub_class_t;

/**
 * @brief Register a sub-class with the composite dispatcher.
 *
 * Call before USBD_Init / platform_usb_composite_init().
 * The first call registers sub-class index 0 (CDC), second call index 1 (HID), …
 */
void usbd_composite_register(const usbd_sub_class_t *entry);

/**
 * @brief Retrieve a sub-class's saved pClassData pointer.
 *
 * Used by platform glue code that calls ST class-specific transmit/send
 * functions which internally dereference pdev->pClassData.
 *
 * @param class_index  Index in the order classes were registered (0-based).
 * @return Pointer saved after that sub-class's Init(), or NULL.
 */
void *usbd_composite_get_state(uint8_t class_index);

/**
 * @brief ST-compatible class driver vtable for USBD_RegisterClass().
 */
extern USBD_ClassTypeDef USBD_Composite;

#endif /* USBD_COMPOSITE_H */
