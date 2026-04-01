#ifndef PLATFORM_SPI_H
#define PLATFORM_SPI_H

/**
 * @file spi.h
 * @brief Platform SPI abstraction — platform layer.
 *
 * Public API is hardware-agnostic: users pass opaque handles and abstract
 * configuration values. Only the HAL implementation includes STM32 headers.
 */

#include <stdint.h>

typedef void *platform_spi_handle_t;

typedef enum {
    PLATFORM_SPI_MODE_0 = 0,
    PLATFORM_SPI_MODE_1 = 1,
    PLATFORM_SPI_MODE_2 = 2,
    PLATFORM_SPI_MODE_3 = 3,
} platform_spi_mode_t;

typedef enum {
    PLATFORM_SPI_BIT_ORDER_MSB_FIRST = 0,
    PLATFORM_SPI_BIT_ORDER_LSB_FIRST = 1,
} platform_spi_bit_order_t;

typedef enum {
    PLATFORM_SPI_BAUDRATE_DIV_2 = 0,
    PLATFORM_SPI_BAUDRATE_DIV_4,
    PLATFORM_SPI_BAUDRATE_DIV_8,
    PLATFORM_SPI_BAUDRATE_DIV_16,
    PLATFORM_SPI_BAUDRATE_DIV_32,
    PLATFORM_SPI_BAUDRATE_DIV_64,
    PLATFORM_SPI_BAUDRATE_DIV_128,
    PLATFORM_SPI_BAUDRATE_DIV_256,
} platform_spi_baudrate_t;

typedef struct {
    platform_spi_mode_t mode;
    platform_spi_bit_order_t bit_order;
    platform_spi_baudrate_t baudrate;
} platform_spi_config_t;

enum {
    PLATFORM_SPI_OK = 0,
    PLATFORM_SPI_ERR_INVALID_ARG = -1,
    PLATFORM_SPI_ERR_IO = -2,
};

platform_spi_handle_t platform_spi1_handle(void);

int platform_spi_init(platform_spi_handle_t handle, const platform_spi_config_t *cfg);

int platform_spi_transceive(platform_spi_handle_t handle,
                            const uint8_t *tx_data,
                            uint8_t *rx_data,
                            uint16_t len,
                            uint32_t timeout_ms);

#endif /* PLATFORM_SPI_H */