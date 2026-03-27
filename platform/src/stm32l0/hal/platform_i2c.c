/**
 * @file platform_i2c.c
 * @brief STM32L0 I2C support
 */

#include "stm32l0xx_hal.h"
#include "board.h"

static I2C_HandleTypeDef g_i2c_handle = {0};

/**
 * @brief Platform_I2C_Init
 *
 * Initialize I2C1 (PB8=SCL, PB9=SDA).
 */
void Platform_I2C_Init(void)
{
    g_i2c_handle.Instance = BOARD_I2C1_INSTANCE;
    g_i2c_handle.Init.Timing = 0x00707CBB;  /* 100 kHz @ 32 MHz SYSCLK */
    g_i2c_handle.Init.OwnAddress1 = 0;
    g_i2c_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    g_i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    g_i2c_handle.Init.OwnAddress2 = 0;
    g_i2c_handle.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    g_i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    g_i2c_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&g_i2c_handle) != HAL_OK)
    {
        return;
    }

    if (HAL_I2CEx_ConfigAnalogFilter(&g_i2c_handle, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        return;
    }

    if (HAL_I2CEx_ConfigDigitalFilter(&g_i2c_handle, 0) != HAL_OK)
    {
        return;
    }
}

/**
 * @brief Platform_I2C_Master_Transmit
 *
 * I2C master write (blocking).
 */
int Platform_I2C_Master_Transmit(uint16_t addr, const uint8_t* data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&g_i2c_handle, addr, (uint8_t*)data, size, timeout);
    return (status == HAL_OK) ? size : -1;
}

/**
 * @brief Platform_I2C_Master_Receive
 *
 * I2C master read (blocking).
 */
int Platform_I2C_Master_Receive(uint16_t addr, uint8_t* data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(&g_i2c_handle, addr, data, size, timeout);
    return (status == HAL_OK) ? size : -1;
}

/**
 * @brief Platform_I2C_Mem_Write
 *
 * I2C memory write (register + data).
 */
int Platform_I2C_Mem_Write(uint16_t addr, uint16_t mem_addr, uint16_t mem_addr_size,
                           const uint8_t* data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&g_i2c_handle, addr, mem_addr, mem_addr_size,
                                                   (uint8_t*)data, size, timeout);
    return (status == HAL_OK) ? size : -1;
}

/**
 * @brief Platform_I2C_Mem_Read
 *
 * I2C memory read (register + data).
 */
int Platform_I2C_Mem_Read(uint16_t addr, uint16_t mem_addr, uint16_t mem_addr_size,
                          uint8_t* data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&g_i2c_handle, addr, mem_addr, mem_addr_size,
                                                  data, size, timeout);
    return (status == HAL_OK) ? size : -1;
}
