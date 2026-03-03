#include "sts40_driver.h"

#include "sts40_driver_config.h"

#include <stddef.h>

static uint8_t Sts40_CommandFromRepeatability(Sts40Repeatability repeatability)
{
    if (repeatability == STS40_REPEATABILITY_LOW)
    {
        return STS40_CMD_MEASURE_LOW;
    }

    if (repeatability == STS40_REPEATABILITY_MEDIUM)
    {
        return STS40_CMD_MEASURE_MEDIUM;
    }

    return STS40_CMD_MEASURE_HIGH;
}

static uint32_t Sts40_WaitMsFromRepeatability(Sts40Repeatability repeatability)
{
    if (repeatability == STS40_REPEATABILITY_LOW)
    {
        return STS40_MEASUREMENT_WAIT_MS_LOW;
    }

    if (repeatability == STS40_REPEATABILITY_MEDIUM)
    {
        return STS40_MEASUREMENT_WAIT_MS_MEDIUM;
    }

    return STS40_MEASUREMENT_WAIT_MS_HIGH;
}

static bool Sts40_IsValidRepeatability(Sts40Repeatability repeatability)
{
    return (repeatability == STS40_REPEATABILITY_LOW) ||
           (repeatability == STS40_REPEATABILITY_MEDIUM) ||
           (repeatability == STS40_REPEATABILITY_HIGH);
}

static Sts40Status Sts40_I2cWriteWithRetry(Sts40Device *device, uint8_t address, const uint8_t *data, uint16_t length)
{
    uint8_t attempt;

    for (attempt = 0; attempt <= device->config.io_retry_count; ++attempt)
    {
        if (device->io.i2c_write(device->io.context, address, data, length) == 0)
        {
            return STS40_STATUS_OK;
        }

        if ((attempt < device->config.io_retry_count) && (device->io.delay_ms != NULL))
        {
            device->io.delay_ms(device->io.context, device->config.io_retry_delay_ms);
        }
    }

    return STS40_STATUS_IO;
}

static Sts40Status Sts40_I2cReadWithRetry(Sts40Device *device, uint8_t address, uint8_t *data, uint16_t length)
{
    uint8_t attempt;

    for (attempt = 0; attempt <= device->config.io_retry_count; ++attempt)
    {
        if (device->io.i2c_read(device->io.context, address, data, length) == 0)
        {
            return STS40_STATUS_OK;
        }

        if ((attempt < device->config.io_retry_count) && (device->io.delay_ms != NULL))
        {
            device->io.delay_ms(device->io.context, device->config.io_retry_delay_ms);
        }
    }

    return STS40_STATUS_IO;
}

void Sts40_ConfigDefaults(Sts40Config *config)
{
    if (config == NULL)
    {
        return;
    }

    config->i2c_address = STS40_I2C_ADDRESS_DEFAULT;
    config->default_repeatability = (Sts40Repeatability)STS40_DRIVER_DEFAULT_REPEATABILITY;
    config->enable_crc_check = (STS40_DRIVER_ENABLE_CRC_CHECK != 0U);
    config->io_retry_count = STS40_DRIVER_ENABLE_RETRY ? STS40_DRIVER_IO_RETRY_COUNT : 0U;
    config->io_retry_delay_ms = STS40_DRIVER_IO_RETRY_DELAY_MS;
}

Sts40Status Sts40_Init(Sts40Device *device, const Sts40Io *io, const Sts40Config *config)
{
    if ((device == NULL) || (io == NULL) || (io->i2c_write == NULL) || (io->i2c_read == NULL))
    {
        return STS40_STATUS_INVALID_ARG;
    }

    device->io = *io;

    if (config != NULL)
    {
        device->config = *config;
    }
    else
    {
        Sts40_ConfigDefaults(&device->config);
    }

    if (!Sts40_IsValidRepeatability(device->config.default_repeatability))
    {
        return STS40_STATUS_INVALID_ARG;
    }

    device->initialized = true;
    return STS40_STATUS_OK;
}

Sts40Status Sts40_SoftReset(Sts40Device *device)
{
    uint8_t command = STS40_CMD_SOFT_RESET;
    Sts40Status status;

    if ((device == NULL) || !device->initialized)
    {
        return STS40_STATUS_NOT_INITIALIZED;
    }

    status = Sts40_I2cWriteWithRetry(device, device->config.i2c_address, &command, 1U);
    if (status != STS40_STATUS_OK)
    {
        return status;
    }

    if (device->io.delay_ms != NULL)
    {
        device->io.delay_ms(device->io.context, STS40_RESET_WAIT_MS);
    }

    return STS40_STATUS_OK;
}

Sts40Status Sts40_GeneralCallReset(Sts40Device *device)
{
    uint8_t command = STS40_GENERAL_CALL_RESET_CMD;

    if ((device == NULL) || !device->initialized)
    {
        return STS40_STATUS_NOT_INITIALIZED;
    }

    if (Sts40_I2cWriteWithRetry(device, STS40_GENERAL_CALL_ADDRESS, &command, 1U) != STS40_STATUS_OK)
    {
        return STS40_STATUS_IO;
    }

    if (device->io.delay_ms != NULL)
    {
        device->io.delay_ms(device->io.context, STS40_RESET_WAIT_MS);
    }

    return STS40_STATUS_OK;
}

Sts40Status Sts40_ReadSerial(Sts40Device *device, uint32_t *serial_number)
{
    uint8_t command = STS40_CMD_READ_SERIAL;
    uint8_t data[6];
    uint16_t serial_msb;
    uint16_t serial_lsb;
    Sts40Status status;

    if ((device == NULL) || (serial_number == NULL) || !device->initialized)
    {
        return STS40_STATUS_INVALID_ARG;
    }

    status = Sts40_I2cWriteWithRetry(device, device->config.i2c_address, &command, 1U);
    if (status != STS40_STATUS_OK)
    {
        return status;
    }

    status = Sts40_I2cReadWithRetry(device, device->config.i2c_address, data, sizeof(data));
    if (status != STS40_STATUS_OK)
    {
        return status;
    }

    if (device->config.enable_crc_check)
    {
        if ((Sts40_Crc8(data, 2U) != data[2]) || (Sts40_Crc8(&data[3], 2U) != data[5]))
        {
            return STS40_STATUS_CRC;
        }
    }

    serial_msb = (uint16_t)((uint16_t)data[0] << 8) | data[1];
    serial_lsb = (uint16_t)((uint16_t)data[3] << 8) | data[4];
    *serial_number = ((uint32_t)serial_msb << 16) | serial_lsb;

    return STS40_STATUS_OK;
}

Sts40Status Sts40_MeasureRaw(Sts40Device *device, Sts40Repeatability repeatability, uint16_t *raw_temperature)
{
    uint8_t command;
    uint8_t data[3];
    Sts40Status status;

    if ((device == NULL) || (raw_temperature == NULL) || !device->initialized)
    {
        return STS40_STATUS_INVALID_ARG;
    }

    if (!Sts40_IsValidRepeatability(repeatability))
    {
        repeatability = device->config.default_repeatability;
    }

    command = Sts40_CommandFromRepeatability(repeatability);

    status = Sts40_I2cWriteWithRetry(device, device->config.i2c_address, &command, 1U);
    if (status != STS40_STATUS_OK)
    {
        return status;
    }

    if (device->io.delay_ms != NULL)
    {
        device->io.delay_ms(device->io.context, Sts40_WaitMsFromRepeatability(repeatability));
    }

    status = Sts40_I2cReadWithRetry(device, device->config.i2c_address, data, sizeof(data));
    if (status != STS40_STATUS_OK)
    {
        return status;
    }

    if (device->config.enable_crc_check)
    {
        if (Sts40_Crc8(data, 2U) != data[2])
        {
            return STS40_STATUS_CRC;
        }
    }

    *raw_temperature = (uint16_t)((uint16_t)data[0] << 8) | data[1];
    return STS40_STATUS_OK;
}

Sts40Status Sts40_MeasureTemperatureC(Sts40Device *device, Sts40Repeatability repeatability, float *temperature_c)
{
    uint16_t raw_temperature;
    Sts40Status status;

    if (temperature_c == NULL)
    {
        return STS40_STATUS_INVALID_ARG;
    }

    status = Sts40_MeasureRaw(device, repeatability, &raw_temperature);
    if (status != STS40_STATUS_OK)
    {
        return status;
    }

    *temperature_c = Sts40_RawToTemperatureC(raw_temperature);
    return STS40_STATUS_OK;
}

Sts40Status Sts40_MeasureTemperatureF(Sts40Device *device, Sts40Repeatability repeatability, float *temperature_f)
{
    uint16_t raw_temperature;
    Sts40Status status;

    if (temperature_f == NULL)
    {
        return STS40_STATUS_INVALID_ARG;
    }

    status = Sts40_MeasureRaw(device, repeatability, &raw_temperature);
    if (status != STS40_STATUS_OK)
    {
        return status;
    }

    *temperature_f = Sts40_RawToTemperatureF(raw_temperature);
    return STS40_STATUS_OK;
}

float Sts40_RawToTemperatureC(uint16_t raw_temperature)
{
    return STS40_DRIVER_TEMP_OFFSET_C +
           (STS40_DRIVER_TEMP_SCALE_NUMERATOR * (float)raw_temperature / STS40_DRIVER_TEMP_SCALE_DENOMINATOR);
}

float Sts40_RawToTemperatureF(uint16_t raw_temperature)
{
    return STS40_DRIVER_TEMP_F_OFFSET +
           (STS40_DRIVER_TEMP_F_SCALE_NUMERATOR * (float)raw_temperature / STS40_DRIVER_TEMP_F_SCALE_DENOMINATOR);
}

uint8_t Sts40_Crc8(const uint8_t *data, uint16_t length)
{
    uint8_t crc = STS40_CRC8_INIT;
    uint16_t index;

    if (data == NULL)
    {
        return 0U;
    }

    for (index = 0; index < length; ++index)
    {
        uint8_t bit;

        crc ^= data[index];

        for (bit = 0; bit < 8U; ++bit)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)((crc << 1U) ^ STS40_CRC8_POLYNOMIAL);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return (uint8_t)(crc ^ STS40_CRC8_FINAL_XOR);
}
