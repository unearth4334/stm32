#ifndef PLATFORM_I2C_H
#define PLATFORM_I2C_H

/**
 * @file i2c.h
 * @brief Platform I2C abstraction — platform layer.
 *
 * Public API is hardware-agnostic: users pass an opaque handle and 7-bit
 * device address. Only the HAL implementation includes STM32 headers.
 */

#include <stdint.h>

typedef void *platform_i2c_handle_t;

enum {
    PLATFORM_I2C_OK = 0,
    PLATFORM_I2C_ERR_INVALID_ARG = -1,
    PLATFORM_I2C_ERR_BUS = -2,
};

/**
 * @brief Returns the board's primary I2C handle.
 */
platform_i2c_handle_t platform_i2c_primary_handle(void);

/**
 * @brief Initializes the board's primary I2C peripheral.
 */
int platform_i2c_init_primary(platform_i2c_handle_t handle, uint32_t clock_hz);

/**
 * @brief Write bytes to an 8-bit register address over I2C.
 */
int platform_i2c_mem_write(platform_i2c_handle_t handle,
                           uint8_t dev_addr_7bit,
                           uint8_t reg,
                           const uint8_t *data,
                           uint16_t len,
                           uint32_t timeout_ms);

/**
 * @brief Read bytes from an 8-bit register address over I2C.
 */
int platform_i2c_mem_read(platform_i2c_handle_t handle,
                          uint8_t dev_addr_7bit,
                          uint8_t reg,
                          uint8_t *data,
                          uint16_t len,
                          uint32_t timeout_ms);

#endif /* PLATFORM_I2C_H */
