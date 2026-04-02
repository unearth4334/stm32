#include "ads7822/ads7822_driver.h"

#include <stddef.h>

#include "board.h"
#include "platform/gpio.h"
#include "platform/spi.h"

#define ADS7822_SPI_TRANSFER_BYTES 2U
#define ADS7822_FULL_SCALE         4096.0f
#define ADS7822_SPI_TIMEOUT_MS     10U

static int ads7822_validate_device(const ads7822_t *dev)
{
    if (dev == NULL ||
        dev->spi == NULL ||
        dev->cs_port == NULL ||
        dev->cs_pin == 0U) {
        return ADS7822_ERR_INVALID_ARG;
    }

    return ADS7822_OK;
}

int ads7822_init(ads7822_t *dev,
                 platform_spi_handle_t spi,
                 void *cs_port,
                 uint32_t cs_pin,
                 float vref_v)
{
    static const platform_spi_config_t spi_cfg = {
        PLATFORM_SPI_MODE_0,
        PLATFORM_SPI_BIT_ORDER_MSB_FIRST,
        PLATFORM_SPI_BAUDRATE_DIV_128,
    };

    if (dev == NULL ||
        spi == NULL ||
        cs_port == NULL ||
        cs_pin == 0U ||
        vref_v <= 0.0f) {
        return ADS7822_ERR_INVALID_ARG;
    }

    dev->spi = spi;
    dev->cs_port = cs_port;
    dev->cs_pin = cs_pin;
    dev->vref_v = vref_v;

    if (platform_spi_init(dev->spi, &spi_cfg) != PLATFORM_SPI_OK) {
        return ADS7822_ERR_IO;
    }

    platform_gpio_init_output(dev->cs_port, dev->cs_pin, PLATFORM_GPIO_SPEED_HIGH);

    platform_gpio_write(dev->cs_port, dev->cs_pin, 1U);

    return ADS7822_OK;
}

int ads7822_init_default(ads7822_t *dev)
{
    platform_gpio_init_output(BOARD_ADS7822_LOAD_SW_PORT,
                              BOARD_ADS7822_LOAD_SW_PIN,
                              PLATFORM_GPIO_SPEED_LOW);
    ads7822_load_switch_enable(1U);

    return ads7822_init(dev,
                        platform_spi1_handle(),
                        BOARD_ADS7822_NCS_PORT,
                        BOARD_ADS7822_NCS_PIN,
                        BOARD_ADS7822_VREF_V);
}

int ads7822_read_raw(ads7822_t *dev, uint16_t *sample)
{
    static const uint8_t tx_buf[ADS7822_SPI_TRANSFER_BYTES] = {0U, 0U};
    uint8_t rx_buf[ADS7822_SPI_TRANSFER_BYTES] = {0U, 0U};
    uint16_t frame;
    int st;

    if (sample == NULL) {
        return ADS7822_ERR_INVALID_ARG;
    }

    st = ads7822_validate_device(dev);
    if (st != ADS7822_OK) {
        return st;
    }

    platform_gpio_write(dev->cs_port, dev->cs_pin, 0U);

    if (platform_spi_transceive(dev->spi,
                                tx_buf,
                                rx_buf,
                                ADS7822_SPI_TRANSFER_BYTES,
                                ADS7822_SPI_TIMEOUT_MS) != PLATFORM_SPI_OK) {
        platform_gpio_write(dev->cs_port, dev->cs_pin, 1U);
        return ADS7822_ERR_IO;
    }

    platform_gpio_write(dev->cs_port, dev->cs_pin, 1U);

    frame = ((uint16_t)rx_buf[0] << 8) | (uint16_t)rx_buf[1];

    /* The captured 16-bit frame presents the 12-bit sample in bits 11:0. */
    *sample = (uint16_t)(frame & 0x0FFFU);
    return ADS7822_OK;
}

int ads7822_read_voltage_v(ads7822_t *dev, float *voltage_v)
{
    uint16_t raw;
    int st;

    if (voltage_v == NULL) {
        return ADS7822_ERR_INVALID_ARG;
    }

    st = ads7822_validate_device(dev);
    if (st != ADS7822_OK) {
        return st;
    }

    st = ads7822_read_raw(dev, &raw);
    if (st != ADS7822_OK) {
        return st;
    }

    *voltage_v = ((float)raw * dev->vref_v) / ADS7822_FULL_SCALE;
    return ADS7822_OK;
}

void ads7822_load_switch_enable(uint8_t enable)
{
    platform_gpio_write(BOARD_ADS7822_LOAD_SW_PORT,
                        BOARD_ADS7822_LOAD_SW_PIN,
                        enable ? BOARD_ADS7822_LOAD_SW_ON_LEVEL : BOARD_ADS7822_LOAD_SW_OFF_LEVEL);
}

void ads7822_power_down(ads7822_t *dev)
{
    if (ads7822_validate_device(dev) != ADS7822_OK) {
        return;
    }

    platform_gpio_write(dev->cs_port, dev->cs_pin, 1U);
}