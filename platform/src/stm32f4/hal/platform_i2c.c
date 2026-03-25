/**
 * @file platform_i2c.c
 * @brief STM32F4 HAL implementation of the platform I2C abstraction.
 */

#include "platform/i2c.h"

#include <stddef.h>

#include "board.h"
#include "stm32f4xx_hal.h"

static I2C_HandleTypeDef s_i2c1;
static uint8_t s_i2c1_ready;

platform_i2c_handle_t platform_i2c_primary_handle(void)
{
    return (platform_i2c_handle_t)&s_i2c1;
}

int platform_i2c_init_primary(platform_i2c_handle_t handle, uint32_t clock_hz)
{
    GPIO_InitTypeDef gpio_cfg = {0};

    if ((handle != (platform_i2c_handle_t)&s_i2c1) || (clock_hz == 0U)) {
        return PLATFORM_I2C_ERR_INVALID_ARG;
    }

    BOARD_I2C1_GPIO_CLK_EN();
    BOARD_I2C1_CLK_EN();

    gpio_cfg.Pin = BOARD_I2C1_SCL_PIN | BOARD_I2C1_SDA_PIN;
    gpio_cfg.Mode = GPIO_MODE_AF_OD;
    gpio_cfg.Pull = GPIO_PULLUP;
    gpio_cfg.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_cfg.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init((GPIO_TypeDef *)BOARD_I2C1_SCL_PORT, &gpio_cfg);

    s_i2c1.Instance = BOARD_I2C1_INSTANCE;
    s_i2c1.Init.ClockSpeed = clock_hz;
    s_i2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    s_i2c1.Init.OwnAddress1 = 0U;
    s_i2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    s_i2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    s_i2c1.Init.OwnAddress2 = 0U;
    s_i2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_i2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&s_i2c1) != HAL_OK) {
        s_i2c1_ready = 0U;
        return PLATFORM_I2C_ERR_BUS;
    }

    s_i2c1_ready = 1U;
    return PLATFORM_I2C_OK;
}

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

    if ((handle == (platform_i2c_handle_t)&s_i2c1) && (s_i2c1_ready == 0U)) {
        return PLATFORM_I2C_ERR_BUS;
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

    if ((handle == (platform_i2c_handle_t)&s_i2c1) && (s_i2c1_ready == 0U)) {
        return PLATFORM_I2C_ERR_BUS;
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
