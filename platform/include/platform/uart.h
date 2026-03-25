#ifndef PLATFORM_UART_H
#define PLATFORM_UART_H

#include <stdint.h>

typedef void *platform_uart_handle_t;

enum {
    PLATFORM_UART_OK = 0,
    PLATFORM_UART_ERR_INVALID_ARG = -1,
    PLATFORM_UART_ERR_IO = -2,
};

/* Returns a handle to the board's primary debug UART. */
platform_uart_handle_t platform_uart_debug_handle(void);

/* Initializes the selected UART handle with the requested baudrate. */
int platform_uart_init(platform_uart_handle_t handle, uint32_t baudrate);

/* Writes raw bytes to UART. */
int platform_uart_write(platform_uart_handle_t handle,
                        const uint8_t *data,
                        uint16_t len,
                        uint32_t timeout_ms);

/* Reads up to len bytes from UART. timeout_ms==0 performs a non-blocking read.
 * Not used when IRQ-driven reception is active (FreeRTOS path). */
int platform_uart_read(platform_uart_handle_t handle,
                       uint8_t *data,
                       uint16_t len,
                       uint16_t *read_len,
                       uint32_t timeout_ms);

/* Callback type invoked from UART RX complete IRQ with each received byte. */
typedef void (*platform_uart_rx_cb_t)(uint8_t byte, void *ctx);

/* Registers a callback to be called from the UART RX IRQ handler.
 * Must be called before platform_uart_start_rx_it(). */
int platform_uart_set_rx_callback(platform_uart_handle_t handle,
                                  platform_uart_rx_cb_t cb,
                                  void *ctx);

/* Arms single-byte interrupt-driven reception. The registered callback
 * is invoked for every received byte; reception is restarted automatically. */
int platform_uart_start_rx_it(platform_uart_handle_t handle);

/* Must be called from the UART peripheral IRQ handler (e.g. USART1_IRQHandler). */
void platform_uart_irq_handler(platform_uart_handle_t handle);

#endif /* PLATFORM_UART_H */