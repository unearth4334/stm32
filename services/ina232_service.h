#ifndef INA232_SERVICE_H
#define INA232_SERVICE_H

#include <stdint.h>

#include "ina232/ina232_driver.h"

#ifndef INA232_SERVICE_DEFAULT_ADDR
#define INA232_SERVICE_DEFAULT_ADDR INA232A_ADDR_A0_GND
#endif

#ifndef INA232_SERVICE_DEFAULT_SHUNT_OHMS
#define INA232_SERVICE_DEFAULT_SHUNT_OHMS 0.1f
#endif

#ifndef INA232_SERVICE_DEFAULT_MAX_CURRENT_A
#define INA232_SERVICE_DEFAULT_MAX_CURRENT_A 3.2f
#endif

int ina232_service_init(platform_i2c_handle_t i2c,
                        uint8_t dev_addr_7bit,
                        float shunt_ohms,
                        float max_expected_current_a);

int ina232_service_read_field(const char *field, char *out, uint16_t out_len);

int ina232_service_write_field(const char *field,
                               const char *value,
                               char *out,
                               uint16_t out_len);

#endif /* INA232_SERVICE_H */
