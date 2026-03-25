/**
 * @file platform_i2c.c
 * @brief STM32F4 HAL implementation of the platform I2C abstraction.
 */

#include "platform/i2c.h"

#include <stddef.h>

#include "stm32f4xx_hal.h"

int platform_i2c_mem_write(platform_i2c_handle_t handle,
                           uint8_t dev_addr_7bit,
                           uint8_t reg,
                           const uint8_t *data,
                           uint16_t len,
                           uint32_t timeout_ms)
{
    HAL_StatusTypeDef st;
    I2C_HandleTypeDef *h;

    if ((handle == NULL) || ((data == NULL) && (len != 0U))) {
        return PLATFORM_I2C_ERR_INVALID_ARG;
    }

    h = (I2C_HandleTypeDef *)handle;
    st = HAL_I2C_Mem_Write(h,
                           (uint16_t)(dev_addr_7bit << 1),
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           (uint8_t *)data,
                           len,
                           timeout_ms);

    return (st == HAL_OK) ? PLATFORM_I2C_OK : PLATFORM_I2C_ERR_BUS;
}

int platform_i2c_mem_read(platform_i2c_handle_t handle,
                          uint8_t dev_addr_7bit,
                          uint8_t reg,
                          uint8_t *data,
                          uint16_t len,
                          uint32_t timeout_ms)
{
    HAL_StatusTypeDef st;
    I2C_HandleTypeDef *h;

    if ((handle == NULL) || ((data == NULL) && (len != 0U))) {
        return PLATFORM_I2C_ERR_INVALID_ARG;
    }

    h = (I2C_HandleTypeDef *)handle;
    st = HAL_I2C_Mem_Read(h,
                          (uint16_t)(dev_addr_7bit << 1),
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          data,
                          len,
                          timeout_ms);

    return (st == HAL_OK) ? PLATFORM_I2C_OK : PLATFORM_I2C_ERR_BUS;
}
