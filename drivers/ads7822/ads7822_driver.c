#include "ads7822/ads7822_driver.h"

#include <stddef.h>

#include "platform/gpio.h"

#define ADS7822_WARMUP_CLOCKS 2U
#define ADS7822_DATA_BITS     12U
#define ADS7822_FULL_SCALE    4096.0f

static int ads7822_validate_device(const ads7822_t *dev)
{
    if (dev == NULL ||
        dev->cs_port == NULL ||
        dev->clk_port == NULL ||
        dev->dout_port == NULL ||
        dev->cs_pin == 0U ||
        dev->clk_pin == 0U ||
        dev->dout_pin == 0U) {
        return ADS7822_ERR_INVALID_ARG;
    }

    return ADS7822_OK;
}

static void ads7822_clock_falling_edge(ads7822_t *dev)
{
    platform_gpio_write(dev->clk_port, dev->clk_pin, 1U);
    platform_gpio_write(dev->clk_port, dev->clk_pin, 0U);
}

int ads7822_init(ads7822_t *dev,
                 void *cs_port,
                 uint32_t cs_pin,
                 void *clk_port,
                 uint32_t clk_pin,
                 void *dout_port,
                 uint32_t dout_pin,
                 float vref_v)
{
    if (dev == NULL ||
        cs_port == NULL ||
        clk_port == NULL ||
        dout_port == NULL ||
        cs_pin == 0U ||
        clk_pin == 0U ||
        dout_pin == 0U ||
        vref_v <= 0.0f) {
        return ADS7822_ERR_INVALID_ARG;
    }

    dev->cs_port = cs_port;
    dev->cs_pin = cs_pin;
    dev->clk_port = clk_port;
    dev->clk_pin = clk_pin;
    dev->dout_port = dout_port;
    dev->dout_pin = dout_pin;
    dev->vref_v = vref_v;

    platform_gpio_init_output(dev->cs_port, dev->cs_pin, PLATFORM_GPIO_SPEED_HIGH);
    platform_gpio_init_output(dev->clk_port, dev->clk_pin, PLATFORM_GPIO_SPEED_HIGH);
    platform_gpio_init_input(dev->dout_port, dev->dout_pin);

    platform_gpio_write(dev->clk_port, dev->clk_pin, 0U);
    platform_gpio_write(dev->cs_port, dev->cs_pin, 1U);

    return ADS7822_OK;
}

int ads7822_read_raw(ads7822_t *dev, uint16_t *sample)
{
    uint16_t raw = 0U;
    uint32_t i;
    int st;

    if (sample == NULL) {
        return ADS7822_ERR_INVALID_ARG;
    }

    st = ads7822_validate_device(dev);
    if (st != ADS7822_OK) {
        return st;
    }

    platform_gpio_write(dev->cs_port, dev->cs_pin, 0U);

    for (i = 0U; i < ADS7822_WARMUP_CLOCKS; ++i) {
        ads7822_clock_falling_edge(dev);
    }

    for (i = 0U; i < ADS7822_DATA_BITS; ++i) {
        ads7822_clock_falling_edge(dev);
        raw = (uint16_t)((raw << 1) | (uint16_t)(platform_gpio_read(dev->dout_port, dev->dout_pin) & 0x01U));
    }

    platform_gpio_write(dev->cs_port, dev->cs_pin, 1U);

    *sample = raw;
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

void ads7822_power_down(ads7822_t *dev)
{
    if (ads7822_validate_device(dev) != ADS7822_OK) {
        return;
    }

    platform_gpio_write(dev->cs_port, dev->cs_pin, 1U);
    platform_gpio_write(dev->clk_port, dev->clk_pin, 0U);
}