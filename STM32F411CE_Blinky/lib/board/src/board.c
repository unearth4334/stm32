#include "board.h"

#include "board_config.h"
#include "stm32f4xx_hal.h"

static I2C_HandleTypeDef board_i2c_handle;

void Board_SystemClock_Config(void)
{
}

void Board_Init(void)
{
    HAL_Init();
    Board_SystemClock_Config();
}

void Board_DelayMs(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

HAL_StatusTypeDef Board_I2cInit(void)
{
    board_i2c_handle.Instance = BOARD_I2C_INSTANCE;
    board_i2c_handle.Init.ClockSpeed = BOARD_I2C_TIMING_HZ;
    board_i2c_handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
    board_i2c_handle.Init.OwnAddress1 = 0U;
    board_i2c_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    board_i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    board_i2c_handle.Init.OwnAddress2 = 0U;
    board_i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    board_i2c_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    return HAL_I2C_Init(&board_i2c_handle);
}

I2C_HandleTypeDef *Board_I2cHandle(void)
{
    return &board_i2c_handle;
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef gpio_init = {0};

    if ((hi2c == NULL) || (hi2c->Instance != BOARD_I2C_INSTANCE))
    {
        return;
    }

    BOARD_I2C_GPIO_CLK_ENABLE();
    BOARD_I2C_CLK_ENABLE();

    gpio_init.Pin = BOARD_I2C_SCL_PIN | BOARD_I2C_SDA_PIN;
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = BOARD_I2C_AF;

    HAL_GPIO_Init(BOARD_I2C_GPIO_PORT, &gpio_init);
}
