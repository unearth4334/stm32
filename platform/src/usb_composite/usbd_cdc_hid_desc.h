/**
 * @file usbd_cdc_hid_desc.h
 * @brief Combined CDC-ACM + HID Keyboard USB configuration descriptor.
 *
 * Endpoint assignments (logical USB addresses, same for F4 and L0):
 *
 *   CDC Communication Interface (0):
 *     CDC_CMD_EP  = 0x82  EP2_IN  interrupt  8 B   notification
 *
 *   CDC Data Interface (1):
 *     CDC_OUT_EP  = 0x01  EP1_OUT bulk       64 B  host→device data
 *     CDC_IN_EP   = 0x81  EP1_IN  bulk       64 B  device→host data
 *
 *   HID Keyboard Interface (2):
 *     HID_EPIN_ADDR = 0x83  EP3_IN  interrupt  8 B   boot keyboard report
 *
 * The IAD groups the two CDC interfaces so Windows / macOS correctly
 * identifies them as a single CDC-ACM function.
 *
 * Device class must be set to MISC / COMMON / IAD in the device descriptor
 * when an IAD is present (see usbd_cdc_hid_desc.c).
 */

#ifndef USBD_CDC_HID_DESC_H
#define USBD_CDC_HID_DESC_H

#include <stdint.h>
#include "usbd_def.h"

/* Total configuration descriptor length in bytes. */
#define USBD_CDC_HID_CONFIG_DESC_SIZE   100U

/* Function returns a pointer to the static configuration descriptor buffer. */
uint8_t *usbd_cdc_hid_get_fs_config_desc(uint16_t *length);

/* ST-compatible descriptors callbacks object for USBD_Init(). */
extern USBD_DescriptorsTypeDef CDC_HID_Desc;

#endif /* USBD_CDC_HID_DESC_H */
