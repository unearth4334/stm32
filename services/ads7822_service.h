#ifndef ADS7822_SERVICE_H
#define ADS7822_SERVICE_H

#include <stdint.h>

int ads7822_service_init(void);

int ads7822_service_read_raw(uint16_t *sample);

int ads7822_service_read_voltage_v(float *voltage_v);

#endif /* ADS7822_SERVICE_H */