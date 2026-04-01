#ifndef ADS7822_DRIVER_H
#define ADS7822_DRIVER_H

/**
 * @file ads7822_driver.h
 * @brief Portable ADS7822 sampling ADC driver.
 *
 * The ADS7822 uses a simple clocked serial interface with chip-select,
 * data clock, and data output. This driver bit-bangs that interface through
 * the platform GPIO abstraction, keeping the driver layer hardware-agnostic.
 */

#include <stdint.h>

typedef enum {
    ADS7822_OK = 0,
    ADS7822_ERR_INVALID_ARG = -1,
} ads7822_status_t;

typedef struct {
    void *cs_port;
    uint32_t cs_pin;
    void *clk_port;
    uint32_t clk_pin;
    void *dout_port;
    uint32_t dout_pin;
    float vref_v;
} ads7822_t;

int ads7822_init(ads7822_t *dev,
                 void *cs_port,
                 uint32_t cs_pin,
                 void *clk_port,
                 uint32_t clk_pin,
                 void *dout_port,
                 uint32_t dout_pin,
                 float vref_v);

int ads7822_read_raw(ads7822_t *dev, uint16_t *sample);

int ads7822_read_voltage_v(ads7822_t *dev, float *voltage_v);

void ads7822_power_down(ads7822_t *dev);

#endif /* ADS7822_DRIVER_H */