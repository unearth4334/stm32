#include "board.h"

#include "board_config.h"
#include "stm32f4xx_hal.h"

static I2C_HandleTypeDef board_i2c_handle;
static UART_HandleTypeDef board_uart_handle;

void Board_SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc_init = {0};
    RCC_ClkInitTypeDef clk_init = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc_init.HSIState = RCC_HSI_ON;
    osc_init.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc_init.PLL.PLLState = RCC_PLL_ON;
    osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc_init.PLL.PLLM = 16U;
    osc_init.PLL.PLLN = 336U;
    osc_init.PLL.PLLP = RCC_PLLP_DIV4;
    osc_init.PLL.PLLQ = 7U;

    (void)HAL_RCC_OscConfig(&osc_init);

    clk_init.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk_init.APB1CLKDivider = RCC_HCLK_DIV2;
    clk_init.APB2CLKDivider = RCC_HCLK_DIV1;

    (void)HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_2);
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

HAL_StatusTypeDef Board_UartInit(void)
{
    board_uart_handle.Instance = BOARD_CONSOLE_UART_INSTANCE;
    board_uart_handle.Init.BaudRate = BOARD_CONSOLE_UART_BAUDRATE;
    board_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    board_uart_handle.Init.StopBits = UART_STOPBITS_1;
    board_uart_handle.Init.Parity = UART_PARITY_NONE;
    board_uart_handle.Init.Mode = UART_MODE_TX_RX;
    board_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    board_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;

    return HAL_UART_Init(&board_uart_handle);
}

UART_HandleTypeDef *Board_UartHandle(void)
{
    return &board_uart_handle;
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

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init = {0};

    if ((huart == NULL) || (huart->Instance != BOARD_CONSOLE_UART_INSTANCE))
    {
        return;
    }

    BOARD_CONSOLE_UART_GPIO_CLK_ENABLE();
    BOARD_CONSOLE_UART_CLK_ENABLE();

    gpio_init.Pin = BOARD_CONSOLE_UART_TX_PIN | BOARD_CONSOLE_UART_RX_PIN;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = BOARD_CONSOLE_UART_AF;

    HAL_GPIO_Init(BOARD_CONSOLE_UART_GPIO_PORT, &gpio_init);
}
