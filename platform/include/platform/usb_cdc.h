#ifndef PLATFORM_USB_CDC_H
#define PLATFORM_USB_CDC_H

#include <stdint.h>

typedef void (*platform_usb_cdc_rx_cb_t)(uint8_t byte, void *ctx);

int platform_usb_cdc_init(void);
int platform_usb_cdc_write(const uint8_t *data, uint16_t len, uint32_t timeout_ms);
int platform_usb_cdc_set_rx_callback(platform_usb_cdc_rx_cb_t cb, void *ctx);

/* Called by USB interrupt handler in stm32f4xx_it.c */
void platform_usb_cdc_irq_handler(void);

/* Called from USB CDC receive callback implementation. */
void platform_usb_cdc_on_rx_data(const uint8_t *buf, uint32_t len);

#endif /* PLATFORM_USB_CDC_H */
