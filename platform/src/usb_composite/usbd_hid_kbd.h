/**
 * @file usbd_hid_kbd.h
 * @brief USB HID Boot Keyboard class driver.
 *
 * Implements a standard USB HID Boot Protocol keyboard (8-byte reports).
 * Designed for use inside the composite dispatcher; Init() uses a static
 * state struct so the pointer survives pClassData swaps.
 *
 * Endpoint assignment (must match the composite config descriptor):
 *   HID_EPIN_ADDR  = 0x83  (EP3 IN, interrupt, 8 bytes, 10 ms interval)
 *
 * Report format (Boot Keyboard, 8 bytes):
 *   Byte 0 : modifier bitmask  (see HID_MOD_* constants)
 *   Byte 1 : reserved (always 0)
 *   Byte 2-7 : up to 6 simultaneous keycodes (see HID_KEY_* constants)
 */

#ifndef USBD_HID_KBD_H
#define USBD_HID_KBD_H

#include <stdint.h>
#include "usbd_def.h"

/* ── Endpoint ─────────────────────────────────────────────────────────── */
#define HID_EPIN_ADDR           0x83U
#define HID_EPIN_SIZE           0x08U   /* 8-byte boot keyboard report */
#define HID_FS_BINTERVAL        0x0AU   /* 10 ms polling interval */

/* ── Modifier key bitmask (Byte 0 of report) ───────────────────────────── */
#define HID_MOD_LCTRL           0x01U
#define HID_MOD_LSHIFT          0x02U
#define HID_MOD_LALT            0x04U
#define HID_MOD_LGUI            0x08U
#define HID_MOD_RCTRL           0x10U
#define HID_MOD_RSHIFT          0x20U
#define HID_MOD_RALT            0x40U
#define HID_MOD_RGUI            0x80U

/* ── Common keycodes ───────────────────────────────────────────────────── */
#define HID_KEY_NONE            0x00U
#define HID_KEY_A               0x04U
#define HID_KEY_B               0x05U
#define HID_KEY_C               0x06U
#define HID_KEY_D               0x07U
#define HID_KEY_E               0x08U
#define HID_KEY_F               0x09U
#define HID_KEY_G               0x0AU
#define HID_KEY_H               0x0BU
#define HID_KEY_I               0x0CU
#define HID_KEY_J               0x0DU
#define HID_KEY_K               0x0EU
#define HID_KEY_L               0x0FU
#define HID_KEY_M               0x10U
#define HID_KEY_N               0x11U
#define HID_KEY_O               0x12U
#define HID_KEY_P               0x13U
#define HID_KEY_Q               0x14U
#define HID_KEY_R               0x15U
#define HID_KEY_S               0x16U
#define HID_KEY_T               0x17U
#define HID_KEY_U               0x18U
#define HID_KEY_V               0x19U
#define HID_KEY_W               0x1AU
#define HID_KEY_X               0x1BU
#define HID_KEY_Y               0x1CU
#define HID_KEY_Z               0x1DU
#define HID_KEY_1               0x1EU
#define HID_KEY_2               0x1FU
#define HID_KEY_3               0x20U
#define HID_KEY_4               0x21U
#define HID_KEY_5               0x22U
#define HID_KEY_6               0x23U
#define HID_KEY_7               0x24U
#define HID_KEY_8               0x25U
#define HID_KEY_9               0x26U
#define HID_KEY_0               0x27U
#define HID_KEY_ENTER           0x28U
#define HID_KEY_ESCAPE          0x29U
#define HID_KEY_BACKSPACE       0x2AU
#define HID_KEY_TAB             0x2BU
#define HID_KEY_SPACE           0x2CU
#define HID_KEY_MINUS           0x2DU
#define HID_KEY_EQUAL           0x2EU
#define HID_KEY_LBRACKET        0x2FU
#define HID_KEY_RBRACKET        0x30U
#define HID_KEY_BACKSLASH       0x31U
#define HID_KEY_F1              0x3AU
#define HID_KEY_F2              0x3BU
#define HID_KEY_F3              0x3CU
#define HID_KEY_F4              0x3DU
#define HID_KEY_F5              0x3EU
#define HID_KEY_F6              0x3FU
#define HID_KEY_F7              0x40U
#define HID_KEY_F8              0x41U
#define HID_KEY_F9              0x42U
#define HID_KEY_F10             0x43U
#define HID_KEY_F11             0x44U
#define HID_KEY_F12             0x45U
#define HID_KEY_UP              0x52U
#define HID_KEY_DOWN            0x51U
#define HID_KEY_LEFT            0x50U
#define HID_KEY_RIGHT           0x4FU

/* ── Driver state ──────────────────────────────────────────────────────── */
typedef enum {
    HID_KBD_IDLE = 0U,
    HID_KBD_BUSY
} USBD_HID_KBD_StateTypeDef;

typedef struct {
    uint32_t                   protocol;
    uint32_t                   idle_state;
    uint32_t                   alt_setting;
    USBD_HID_KBD_StateTypeDef  state;
} USBD_HID_KBD_HandleTypeDef;

/* ── Public class driver vtable ─────────────────────────────────────────── */
extern USBD_ClassTypeDef USBD_HID_Keyboard;

/* ── Send a raw 8-byte boot keyboard report ─────────────────────────────── */
uint8_t USBD_HID_KBD_SendReport(USBD_HandleTypeDef *pdev,
                                uint8_t *report,
                                uint16_t len);

#endif /* USBD_HID_KBD_H */
