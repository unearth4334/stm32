/**
 * @file main.c
 * @brief STM32F411CE composite CDC+HID demo.
 *
 * Demonstrates both interfaces simultaneously:
 *   - CDC channel: echoes every received byte back to the host
 *   - HID channel: types "hi" via keyboard report every 5 seconds
 *   - LED:         blinks at ~1 Hz to show the device is alive
 *
 * Build with USE_USB_COMPOSITE=ON, STM32_USE_FREERTOS=OFF.
 */

#include "board.h"
#include "platform/usb_composite.h"

extern void platform_init(void);

/* HID boot-protocol key codes (USB HID Usage Table 1.12, page 53) */
#define HID_KEY_H  0x0BU   /* 'h' */
#define HID_KEY_I  0x0CU   /* 'i' */

/* ── CDC receive callback (ISR context) ─────────────────────────────── */

static void on_cdc_rx(uint8_t ch, void *ctx)
{
    (void)ctx;
    /* Echo every received byte back over the CDC channel */
    platform_usb_composite_cdc_write(&ch, 1U, 10U);
}

/* ── Helpers ─────────────────────────────────────────────────────────── */

static void led_set(int on)
{
    HAL_GPIO_WritePin(BOARD_LED_PORT, BOARD_LED_PIN,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void hid_type_key(uint8_t keycode)
{
    platform_usb_composite_hid_key_press(0x00U, keycode);
    HAL_Delay(20U);
    platform_usb_composite_hid_key_release();
    HAL_Delay(20U);
}

/* ── Entry point ─────────────────────────────────────────────────────── */

int main(void)
{
    platform_init();
    board_init();

    platform_usb_composite_init();
    platform_usb_composite_set_rx_callback(on_cdc_rx, NULL);

    uint32_t last_hid_ms  = 0U;
    uint32_t last_blink   = 0U;
    int      led_state    = 0;

    while (1) {
        uint32_t now = HAL_GetTick();

        /* Toggle LED every 500 ms */
        if ((now - last_blink) >= 500U) {
            last_blink = now;
            led_state  = !led_state;
            led_set(led_state);
        }

        /* Send HID keystroke pair ("hi") every 5 seconds */
        if ((now - last_hid_ms) >= 5000U) {
            last_hid_ms = now;
            hid_type_key(HID_KEY_H);
            hid_type_key(HID_KEY_I);
        }
    }
}
