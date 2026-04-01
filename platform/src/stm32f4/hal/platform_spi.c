#include "platform/spi.h"

#include <stddef.h>

#include "board.h"
#include "stm32f4xx_hal.h"

static SPI_HandleTypeDef s_spi1;
static uint8_t s_spi1_ready;

platform_spi_handle_t platform_spi1_handle(void)
{
    return (platform_spi_handle_t)&s_spi1;
}

static int platform_spi_is_spi1_handle(platform_spi_handle_t handle)
{
    return (handle == (platform_spi_handle_t)&s_spi1) ? 1 : 0;
}

static uint32_t platform_spi_hal_polarity(platform_spi_mode_t mode)
{
    return ((mode == PLATFORM_SPI_MODE_2) || (mode == PLATFORM_SPI_MODE_3)) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
}

static uint32_t platform_spi_hal_phase(platform_spi_mode_t mode)
{
    return ((mode == PLATFORM_SPI_MODE_1) || (mode == PLATFORM_SPI_MODE_3)) ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
}

static uint32_t platform_spi_hal_first_bit(platform_spi_bit_order_t bit_order)
{
    return (bit_order == PLATFORM_SPI_BIT_ORDER_LSB_FIRST) ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;
}

static uint32_t platform_spi_hal_prescaler(platform_spi_baudrate_t baudrate)
{
    static const uint32_t prescalers[] = {
        SPI_BAUDRATEPRESCALER_2,
        SPI_BAUDRATEPRESCALER_4,
        SPI_BAUDRATEPRESCALER_8,
        SPI_BAUDRATEPRESCALER_16,
        SPI_BAUDRATEPRESCALER_32,
        SPI_BAUDRATEPRESCALER_64,
        SPI_BAUDRATEPRESCALER_128,
        SPI_BAUDRATEPRESCALER_256,
    };

    return prescalers[(uint8_t)baudrate & 0x07U];
}

int platform_spi_init(platform_spi_handle_t handle, const platform_spi_config_t *cfg)
{
    GPIO_InitTypeDef gpio_cfg = {0};

    if ((!platform_spi_is_spi1_handle(handle)) || (cfg == NULL)) {
        return PLATFORM_SPI_ERR_INVALID_ARG;
    }

    if ((uint8_t)cfg->mode > (uint8_t)PLATFORM_SPI_MODE_3 ||
        (uint8_t)cfg->bit_order > (uint8_t)PLATFORM_SPI_BIT_ORDER_LSB_FIRST ||
        (uint8_t)cfg->baudrate > (uint8_t)PLATFORM_SPI_BAUDRATE_DIV_256) {
        return PLATFORM_SPI_ERR_INVALID_ARG;
    }

    BOARD_SPI1_GPIO_CLK_EN();
    BOARD_SPI1_CLK_EN();

    gpio_cfg.Pin = BOARD_SPI1_SCK_PIN | BOARD_SPI1_MISO_PIN | BOARD_SPI1_MOSI_PIN;
    gpio_cfg.Mode = GPIO_MODE_AF_PP;
    gpio_cfg.Pull = GPIO_NOPULL;
    gpio_cfg.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_cfg.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init((GPIO_TypeDef *)BOARD_SPI1_SCK_PORT, &gpio_cfg);

    s_spi1.Instance = BOARD_SPI1_INSTANCE;
    s_spi1.Init.Mode = SPI_MODE_MASTER;
    s_spi1.Init.Direction = SPI_DIRECTION_2LINES;
    s_spi1.Init.DataSize = SPI_DATASIZE_8BIT;
    s_spi1.Init.CLKPolarity = platform_spi_hal_polarity(cfg->mode);
    s_spi1.Init.CLKPhase = platform_spi_hal_phase(cfg->mode);
    s_spi1.Init.NSS = SPI_NSS_SOFT;
    s_spi1.Init.BaudRatePrescaler = platform_spi_hal_prescaler(cfg->baudrate);
    s_spi1.Init.FirstBit = platform_spi_hal_first_bit(cfg->bit_order);
    s_spi1.Init.TIMode = SPI_TIMODE_DISABLE;
    s_spi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    s_spi1.Init.CRCPolynomial = 7U;

    if (HAL_SPI_Init(&s_spi1) != HAL_OK) {
        s_spi1_ready = 0U;
        return PLATFORM_SPI_ERR_IO;
    }

    s_spi1_ready = 1U;
    return PLATFORM_SPI_OK;
}

int platform_spi_transceive(platform_spi_handle_t handle,
                            const uint8_t *tx_data,
                            uint8_t *rx_data,
                            uint16_t len,
                            uint32_t timeout_ms)
{
    if ((!platform_spi_is_spi1_handle(handle)) ||
        ((tx_data == NULL) && (len != 0U)) ||
        ((rx_data == NULL) && (len != 0U)) ||
        (s_spi1_ready == 0U)) {
        return PLATFORM_SPI_ERR_INVALID_ARG;
    }

    if (len == 0U) {
        return PLATFORM_SPI_OK;
    }

    if (HAL_SPI_TransmitReceive(&s_spi1, (uint8_t *)tx_data, rx_data, len, timeout_ms) != HAL_OK) {
        return PLATFORM_SPI_ERR_IO;
    }

    return PLATFORM_SPI_OK;
}