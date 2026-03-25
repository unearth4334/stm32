#include "platform/uart.h"

#include <stddef.h>

#include "board.h"
#include "stm32f4xx_hal.h"

static UART_HandleTypeDef s_debug_uart;
static uint8_t s_debug_uart_ready;

/* Single-byte receive buffer for interrupt-driven reception */
static uint8_t s_rx_byte;
static platform_uart_rx_cb_t s_rx_cb;
static void *s_rx_cb_ctx;

platform_uart_handle_t platform_uart_debug_handle(void)
{
    return (platform_uart_handle_t)&s_debug_uart;
}

static int platform_uart_is_debug_handle(platform_uart_handle_t handle)
{
    return (handle == (platform_uart_handle_t)&s_debug_uart) ? 1 : 0;
}

int platform_uart_init(platform_uart_handle_t handle, uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_cfg = {0};

    if ((!platform_uart_is_debug_handle(handle)) || (baudrate == 0U)) {
        return PLATFORM_UART_ERR_INVALID_ARG;
    }

    BOARD_DEBUG_UART_GPIO_CLK_EN();
    BOARD_DEBUG_UART_CLK_EN();

    gpio_cfg.Pin = BOARD_DEBUG_UART_TX_PIN | BOARD_DEBUG_UART_RX_PIN;
    gpio_cfg.Mode = GPIO_MODE_AF_PP;
    gpio_cfg.Pull = GPIO_PULLUP;
    gpio_cfg.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_cfg.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init((GPIO_TypeDef *)BOARD_DEBUG_UART_TX_PORT, &gpio_cfg);

    s_debug_uart.Instance = BOARD_DEBUG_UART;
    s_debug_uart.Init.BaudRate = baudrate;
    s_debug_uart.Init.WordLength = UART_WORDLENGTH_8B;
    s_debug_uart.Init.StopBits = UART_STOPBITS_1;
    s_debug_uart.Init.Parity = UART_PARITY_NONE;
    s_debug_uart.Init.Mode = UART_MODE_TX_RX;
    s_debug_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    s_debug_uart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&s_debug_uart) != HAL_OK) {
        s_debug_uart_ready = 0U;
        return PLATFORM_UART_ERR_IO;
    }

    /* Configure USART1 IRQ — priority must be within FreeRTOS syscall range */
    HAL_NVIC_SetPriority(BOARD_DEBUG_UART_IRQn, BOARD_DEBUG_UART_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(BOARD_DEBUG_UART_IRQn);

    s_debug_uart_ready = 1U;
    return PLATFORM_UART_OK;
}

int platform_uart_write(platform_uart_handle_t handle,
                        const uint8_t *data,
                        uint16_t len,
                        uint32_t timeout_ms)
{
    if ((!platform_uart_is_debug_handle(handle)) ||
        ((data == NULL) && (len != 0U)) ||
        (s_debug_uart_ready == 0U)) {
        return PLATFORM_UART_ERR_INVALID_ARG;
    }

    if (HAL_UART_Transmit(&s_debug_uart, (uint8_t *)data, len, timeout_ms) != HAL_OK) {
        return PLATFORM_UART_ERR_IO;
    }

    return PLATFORM_UART_OK;
}

int platform_uart_read(platform_uart_handle_t handle,
                       uint8_t *data,
                       uint16_t len,
                       uint16_t *read_len,
                       uint32_t timeout_ms)
{
    HAL_StatusTypeDef st;

    if ((!platform_uart_is_debug_handle(handle)) ||
        (data == NULL) ||
        (read_len == NULL) ||
        (s_debug_uart_ready == 0U)) {
        return PLATFORM_UART_ERR_INVALID_ARG;
    }

    *read_len = 0U;

    if (len == 0U) {
        return PLATFORM_UART_OK;
    }

    st = HAL_UART_Receive(&s_debug_uart, data, len, timeout_ms);
    if (st == HAL_TIMEOUT) {
        return PLATFORM_UART_OK;
    }

    if (st != HAL_OK) {
        return PLATFORM_UART_ERR_IO;
    }

    *read_len = len;
    return PLATFORM_UART_OK;
}

int platform_uart_set_rx_callback(platform_uart_handle_t handle,
                                  platform_uart_rx_cb_t cb,
                                  void *ctx)
{
    if (!platform_uart_is_debug_handle(handle)) {
        return PLATFORM_UART_ERR_INVALID_ARG;
    }

    s_rx_cb = cb;
    s_rx_cb_ctx = ctx;
    return PLATFORM_UART_OK;
}

int platform_uart_start_rx_it(platform_uart_handle_t handle)
{
    if ((!platform_uart_is_debug_handle(handle)) || (s_debug_uart_ready == 0U)) {
        return PLATFORM_UART_ERR_INVALID_ARG;
    }

    if (HAL_UART_Receive_IT(&s_debug_uart, &s_rx_byte, 1U) != HAL_OK) {
        return PLATFORM_UART_ERR_IO;
    }

    return PLATFORM_UART_OK;
}

void platform_uart_irq_handler(platform_uart_handle_t handle)
{
    if (platform_uart_is_debug_handle(handle)) {
        HAL_UART_IRQHandler(&s_debug_uart);
    }
}

/* Called by HAL after each single-byte IT reception completes. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == BOARD_DEBUG_UART) {
        if (s_rx_cb != NULL) {
            s_rx_cb(s_rx_byte, s_rx_cb_ctx);
        }
        /* Restart single-byte reception for the next character */
        (void)HAL_UART_Receive_IT(&s_debug_uart, &s_rx_byte, 1U);
    }
}