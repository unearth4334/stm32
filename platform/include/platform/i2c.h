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

typedef struct {
    uint8_t ready;
    uint8_t bus_idle;
    uint8_t scl_high;
    uint8_t sda_high;
    uint8_t transaction_active;
    uint8_t predictor_confident;
    uint8_t predictor_samples;
    uint8_t in_predicted_window;
    uint32_t start_count;
    uint32_t stop_count;
    uint32_t repeated_start_count;
    uint32_t scl_rise_count;
    uint32_t scl_fall_count;
    uint32_t sda_rise_count;
    uint32_t sda_fall_count;
    uint32_t last_activity_ms;
    uint32_t last_start_ms;
    uint32_t last_stop_ms;
    uint32_t last_scl_edge_ms;
    uint32_t last_sda_edge_ms;
    uint32_t idle_guard_ms;
    uint32_t interval_ms;
    uint32_t jitter_ms;
    uint32_t transaction_span_ms;
    uint32_t next_window_open_ms;
    uint32_t next_window_close_ms;
} platform_i2c_bus_guard_status_t;

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
 * @brief Initializes the passive PB6/PB7 monitor for the primary I2C bus.
 */
int platform_i2c_primary_monitor_init(void);

/**
 * @brief Returns passive bus-guard state for the board's primary I2C bus.
 */
int platform_i2c_primary_bus_guard_status(platform_i2c_bus_guard_status_t *status);

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
