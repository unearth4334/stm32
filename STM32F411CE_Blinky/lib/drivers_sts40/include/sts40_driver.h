#ifndef STS40_DRIVER_H
#define STS40_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    STS40_STATUS_OK = 0,
    STS40_STATUS_INVALID_ARG = -1,
    STS40_STATUS_IO = -2,
    STS40_STATUS_CRC = -3,
    STS40_STATUS_NOT_INITIALIZED = -4
} Sts40Status;

typedef enum
{
    STS40_REPEATABILITY_LOW = 0,
    STS40_REPEATABILITY_MEDIUM = 1,
    STS40_REPEATABILITY_HIGH = 2
} Sts40Repeatability;

typedef int32_t (*Sts40I2cWrite)(void *context, uint8_t address, const uint8_t *data, uint16_t length);
typedef int32_t (*Sts40I2cRead)(void *context, uint8_t address, uint8_t *data, uint16_t length);
typedef void (*Sts40DelayMs)(void *context, uint32_t delay_ms);

typedef struct
{
    void *context;
    Sts40I2cWrite i2c_write;
    Sts40I2cRead i2c_read;
    Sts40DelayMs delay_ms;
} Sts40Io;

typedef struct
{
    uint8_t i2c_address;
    Sts40Repeatability default_repeatability;
    bool enable_crc_check;
    uint8_t io_retry_count;
    uint32_t io_retry_delay_ms;
} Sts40Config;

typedef struct
{
    Sts40Io io;
    Sts40Config config;
    bool initialized;
} Sts40Device;

void Sts40_ConfigDefaults(Sts40Config *config);
Sts40Status Sts40_Init(Sts40Device *device, const Sts40Io *io, const Sts40Config *config);

Sts40Status Sts40_SoftReset(Sts40Device *device);
Sts40Status Sts40_GeneralCallReset(Sts40Device *device);

Sts40Status Sts40_ReadSerial(Sts40Device *device, uint32_t *serial_number);
Sts40Status Sts40_MeasureRaw(Sts40Device *device, Sts40Repeatability repeatability, uint16_t *raw_temperature);
Sts40Status Sts40_MeasureTemperatureC(Sts40Device *device, Sts40Repeatability repeatability, float *temperature_c);
Sts40Status Sts40_MeasureTemperatureF(Sts40Device *device, Sts40Repeatability repeatability, float *temperature_f);

float Sts40_RawToTemperatureC(uint16_t raw_temperature);
float Sts40_RawToTemperatureF(uint16_t raw_temperature);

uint8_t Sts40_Crc8(const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
