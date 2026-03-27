/**
 * @file platform_uart.c
 * @brief STM32L0 UART/USART support
 */

#include "stm32l0xx_hal.h"
#include "board.h"

static UART_HandleTypeDef g_uart_handle = {0};

/**
 * @brief Platform_UART_Init
 *
 * Initialize the debug UART (USART2 on PA2/PA3, 115200 baud).
 */
void Platform_UART_Init(void)
{
    g_uart_handle.Instance = BOARD_DEBUG_UART;
    g_uart_handle.Init.BaudRate = BOARD_DEBUG_UART_BAUD;
    g_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart_handle.Init.StopBits = UART_STOPBITS_1;
    g_uart_handle.Init.Parity = UART_PARITY_NONE;
    g_uart_handle.Init.Mode = UART_MODE_TX_RX;
    g_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;
    g_uart_handle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    g_uart_handle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&g_uart_handle) != HAL_OK)
    {
        // Error_Handler already called from Platform_Init fallback
        return;
    }
}

/**
 * @brief Platform_UART_Transmit
 *
 * Send data via UART (blocking).
 */
int Platform_UART_Transmit(const uint8_t* data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(&g_uart_handle, (uint8_t*)data, size, timeout);
    return (status == HAL_OK) ? size : -1;
}

/**
 * @brief Platform_UART_Receive
 *
 * Receive data via UART (blocking).
 */
int Platform_UART_Receive(uint8_t* data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_UART_Receive(&g_uart_handle, data, size, timeout);
    return (status == HAL_OK) ? size : -1;
}
