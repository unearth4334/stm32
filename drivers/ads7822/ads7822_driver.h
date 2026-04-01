#ifndef ADS7822_DRIVER_H
#define ADS7822_DRIVER_H

/**
 * @file ads7822_driver.h
 * @brief Portable ADS7822 sampling ADC driver.
 *
 * The ADS7822 uses a simple SPI-compatible clocked serial interface.
 * This driver uses the platform SPI abstraction for clocking/data capture
 * and the platform GPIO abstraction for chip-select.
 */

#include <stdint.h>

#include "platform/spi.h"

typedef enum {
    ADS7822_OK = 0,
    ADS7822_ERR_INVALID_ARG = -1,
    ADS7822_ERR_IO = -2,
} ads7822_status_t;

typedef struct {
    platform_spi_handle_t spi;
    void *cs_port;
    uint32_t cs_pin;
    float vref_v;
} ads7822_t;

int ads7822_init(ads7822_t *dev,
                 platform_spi_handle_t spi,
                 void *cs_port,
                 uint32_t cs_pin,
                 float vref_v);

int ads7822_read_raw(ads7822_t *dev, uint16_t *sample);

int ads7822_read_voltage_v(ads7822_t *dev, float *voltage_v);

void ads7822_power_down(ads7822_t *dev);

#endif /* ADS7822_DRIVER_H */