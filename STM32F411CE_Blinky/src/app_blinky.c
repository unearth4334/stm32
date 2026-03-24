#include "app_blinky.h"

#include "board.h"
#include "board_config.h"
#include "console.h"
#include "led_driver.h"
#include "sts40_driver.h"
#include "usb_device.h"
#include "usb_console_port.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    Sts40Device sts40_device;
    bool sts40_ready;
    bool poll_enabled;
    uint32_t poll_period_ms;
    uint32_t next_poll_at_ms;
    uint8_t poll_bar_width;
    float poll_bar_min;
    float poll_bar_max;
    uint8_t sensor_addresses[3];
    uint8_t sensor_count;
    uint32_t next_blink_at_ms;
    uint32_t blink_period_ms;
} AppConsoleContext;

static bool App_IsValidSensorAddress(uint32_t address)
{
    return (address == 0x44U) || (address == 0x45U) || (address == 0x46U);
}

static bool App_HasDuplicateAddress(const uint8_t *addresses, uint8_t count, uint8_t address)
{
    uint8_t i;

    for (i = 0U; i < count; ++i)
    {
        if (addresses[i] == address)
        {
            return true;
        }
    }

    return false;
}

static bool App_ParseAddressList(const char *input, uint8_t *addresses_out, uint8_t *count_out)
{
    char buffer[16];
    const char *p = input;
    uint8_t count = 0U;

    if ((input == NULL) || (addresses_out == NULL) || (count_out == NULL))
    {
        return false;
    }

    if (strcmp(input, "all") == 0)
    {
        addresses_out[0] = 0x44U;
        addresses_out[1] = 0x45U;
        addresses_out[2] = 0x46U;
        *count_out = 3U;
        return true;
    }

    while (*p != '\0')
    {
        uint16_t i = 0U;
        uint32_t parsed_addr = 0U;

        while ((*p != '\0') && (*p != ',') && (i < (sizeof(buffer) - 1U)))
        {
            buffer[i++] = *p++;
        }
        buffer[i] = '\0';

        if ((i == 0U) || !App_ParseU32(buffer, &parsed_addr) || !App_IsValidSensorAddress(parsed_addr))
        {
            return false;
        }

        if ((count >= 3U) || App_HasDuplicateAddress(addresses_out, count, (uint8_t)parsed_addr))
        {
            return false;
        }

        addresses_out[count++] = (uint8_t)parsed_addr;

        if (*p == ',')
        {
            p++;
        }
    }

    if (count == 0U)
    {
        return false;
    }

    *count_out = count;
    return true;
}

static void App_FormatAddressList(const AppConsoleContext *app_context, char *buffer, uint16_t buffer_size)
{
    uint16_t used;
    uint8_t i;

    if ((app_context == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    used = 0U;
    buffer[0] = '\0';

    for (i = 0U; i < app_context->sensor_count; ++i)
    {
        int written;
        const char *separator = (i == 0U) ? "" : ",";

        written = snprintf(&buffer[used],
                           (size_t)(buffer_size - used),
                           "%s0x%02X",
                           separator,
                           app_context->sensor_addresses[i]);
        if ((written <= 0) || ((uint16_t)written >= (buffer_size - used)))
        {
            break;
        }

        used = (uint16_t)(used + (uint16_t)written);
    }
}

static Sts40Status App_ReadTemperatureForAddress(AppConsoleContext *app_context,
                                                 uint8_t address,
                                                 float *temperature_c)
{
    uint8_t previous_address;
    Sts40Status status;

    if ((app_context == NULL) || (temperature_c == NULL))
    {
        return STS40_STATUS_INVALID_ARG;
    }

    previous_address = app_context->sts40_device.config.i2c_address;
    app_context->sts40_device.config.i2c_address = address;

    status = Sts40_MeasureTemperatureC(&app_context->sts40_device,
                                       app_context->sts40_device.config.default_repeatability,
                                       temperature_c);

    app_context->sts40_device.config.i2c_address = previous_address;
    return status;
}

static Sts40Status App_ReadRawForAddress(AppConsoleContext *app_context,
                                         uint8_t address,
                                         uint16_t *raw)
{
    uint8_t previous_address;
    Sts40Status status;

    if ((app_context == NULL) || (raw == NULL))
    {
        return STS40_STATUS_INVALID_ARG;
    }

    previous_address = app_context->sts40_device.config.i2c_address;
    app_context->sts40_device.config.i2c_address = address;

    status = Sts40_MeasureRaw(&app_context->sts40_device,
                              app_context->sts40_device.config.default_repeatability,
                              raw);

    app_context->sts40_device.config.i2c_address = previous_address;
    return status;
}

static Sts40Status App_ReadSerialForAddress(AppConsoleContext *app_context,
                                            uint8_t address,
                                            uint32_t *serial_number)
{
    uint8_t previous_address;
    Sts40Status status;

    if ((app_context == NULL) || (serial_number == NULL))
    {
        return STS40_STATUS_INVALID_ARG;
    }

    previous_address = app_context->sts40_device.config.i2c_address;
    app_context->sts40_device.config.i2c_address = address;

    status = Sts40_ReadSerial(&app_context->sts40_device, serial_number);

    app_context->sts40_device.config.i2c_address = previous_address;
    return status;
}

static void App_MakeTempBar(char *buffer, uint16_t buffer_size, float temp_c, uint8_t width, float temp_min, float temp_max)
{
    uint16_t pos = 0U;
    uint8_t filled_chars = 0U;
    uint8_t i = 0U;

    if ((buffer == NULL) || (buffer_size == 0U) || (width == 0U))
    {
        return;
    }

    float temp_clamped = temp_c;
    if (temp_clamped < temp_min) temp_clamped = temp_min;
    if (temp_clamped > temp_max) temp_clamped = temp_max;

    float ratio = (temp_clamped - temp_min) / (temp_max - temp_min);
    filled_chars = (uint8_t)(ratio * (float)width);
    if (filled_chars > width) filled_chars = width;

    buffer[pos++] = '\t';
    buffer[pos++] = '[';

    for (i = 0U; (i < filled_chars) && ((pos + 3U) < buffer_size); ++i)
    {
        buffer[pos++] = (char)0xE2U;
        buffer[pos++] = (char)0xA0U;
        buffer[pos++] = (char)0xBFU;
    }

    for (; (i < width) && (pos < buffer_size); ++i)
    {
        buffer[pos++] = '-';
    }

    if (pos < buffer_size)
    {
        buffer[pos++] = ']';
    }

    buffer[pos] = '\0';
}

static int32_t App_Sts40_I2cWrite(void *context, uint8_t address, const uint8_t *data, uint16_t length)
{
    I2C_HandleTypeDef *i2c_handle = (I2C_HandleTypeDef *)context;

    if ((i2c_handle == NULL) || (data == NULL))
    {
        return -1;
    }

    return (HAL_I2C_Master_Transmit(i2c_handle,
                                    (uint16_t)(address << 1U),
                                    (uint8_t *)data,
                                    length,
                                    BOARD_I2C_TIMEOUT_MS) == HAL_OK) ? 0 : -1;
}

static int32_t App_Sts40_I2cRead(void *context, uint8_t address, uint8_t *data, uint16_t length)
{
    I2C_HandleTypeDef *i2c_handle = (I2C_HandleTypeDef *)context;

    if ((i2c_handle == NULL) || (data == NULL))
    {
        return -1;
    }

    return (HAL_I2C_Master_Receive(i2c_handle,
                                   (uint16_t)(address << 1U),
                                   data,
                                   length,
                                   BOARD_I2C_TIMEOUT_MS) == HAL_OK) ? 0 : -1;
}

static void App_Sts40_DelayMs(void *context, uint32_t delay_ms)
{
    (void)context;
    Board_DelayMs(delay_ms);
}

static int32_t App_ConsoleReadByte(void *context, uint8_t *byte_out)
{
    (void)context;
    return UsbConsole_ReadByte(byte_out);
}

static int32_t App_ConsoleWrite(void *context, const uint8_t *data, uint16_t length)
{
    (void)context;
    return UsbConsole_Write(data, length);
}

static const char *App_RepeatabilityToString(Sts40Repeatability repeatability)
{
    if (repeatability == STS40_REPEATABILITY_LOW)
    {
        return "low";
    }
    if (repeatability == STS40_REPEATABILITY_MEDIUM)
    {
        return "medium";
    }
    return "high";
}

static bool App_ParseRepeatability(const char *text, Sts40Repeatability *repeatability)
{
    if ((text == NULL) || (repeatability == NULL))
    {
        return false;
    }

    if (strcmp(text, "low") == 0)
    {
        *repeatability = STS40_REPEATABILITY_LOW;
        return true;
    }
    if ((strcmp(text, "med") == 0) || (strcmp(text, "medium") == 0))
    {
        *repeatability = STS40_REPEATABILITY_MEDIUM;
        return true;
    }
    if (strcmp(text, "high") == 0)
    {
        *repeatability = STS40_REPEATABILITY_HIGH;
        return true;
    }

    return false;
}

static bool App_ParseU32(const char *text, uint32_t *value)
{
    char *end_ptr = NULL;
    unsigned long parsed;

    if ((text == NULL) || (value == NULL))
    {
        return false;
    }

    parsed = strtoul(text, &end_ptr, 0);
    if ((end_ptr == text) || (*end_ptr != '\0'))
    {
        return false;
    }

    *value = (uint32_t)parsed;
    return true;
}

static bool App_ParseBarConfig(const char *input, uint32_t *min, uint32_t *width, uint32_t *max)
{
    char buffer[32];
    const char *p = input;
    uint16_t i = 0U;
    uint16_t part = 0U;
    uint32_t values[3] = {0};

    if ((input == NULL) || (min == NULL) || (width == NULL) || (max == NULL))
    {
        return false;
    }

    while (*p != '\0' && part < 3U)
    {
        i = 0U;
        while (*p != '\0' && *p != ',' && i < (sizeof(buffer) - 1U))
        {
            buffer[i++] = *p++;
        }
        buffer[i] = '\0';

        if (!App_ParseU32(buffer, &values[part]))
        {
            return false;
        }

        part++;
        if (*p == ',') p++;
    }

    if (part != 3U)
    {
        return false;
    }

    *min = values[0];
    *width = values[1];
    *max = values[2];

    return true;
}

static void App_CmdHelp(Console *console, void *user_context, uint8_t argc, char **argv)
{
    (void)user_context;
    (void)argc;
    (void)argv;

    Console_WriteLine(console, "Commands:");
    Console_WriteLine(console, "  help");
    Console_WriteLine(console, "  status");
    Console_WriteLine(console, "  read c|f|raw");
    Console_WriteLine(console, "  serial");
    Console_WriteLine(console, "  reset soft|gc");
    Console_WriteLine(console, "  config show");
    Console_WriteLine(console, "  config addr <0x44|0x45|0x46|all|list>");
    Console_WriteLine(console, "  config rep <low|medium|high>");
    Console_WriteLine(console, "  config crc <on|off>");
    Console_WriteLine(console, "  config retry <count> <delay_ms>");
    Console_WriteLine(console, "  poll on <period_ms> [bar <min>,<width>,<max>]");
    Console_WriteLine(console, "  poll bar <min>,<width>,<max>");
    Console_WriteLine(console, "  poll off");
    Console_WriteLine(console, "  led on|off|toggle");
}

static void App_CmdStatus(Console *console, void *user_context, uint8_t argc, char **argv)
{
    AppConsoleContext *app_context = (AppConsoleContext *)user_context;

    (void)argc;
    (void)argv;

    if (app_context == NULL)
    {
        return;
    }

    {
        char address_list[32];
        App_FormatAddressList(app_context, address_list, sizeof(address_list));

        if (app_context->poll_bar_width > 0U)
        {
            int32_t bar_min_int = (int32_t)app_context->poll_bar_min;
            int32_t bar_max_int = (int32_t)app_context->poll_bar_max;
            Console_Printf(console,
                           "sensor=%s addr=%s rep=%s crc=%s retry=%u/%lu poll=%s %lu ms bar=%u(%ld-%ld\u00B0C)\r\n",
                           app_context->sts40_ready ? "ready" : "not-ready",
                           address_list,
                           App_RepeatabilityToString(app_context->sts40_device.config.default_repeatability),
                           app_context->sts40_device.config.enable_crc_check ? "on" : "off",
                           app_context->sts40_device.config.io_retry_count,
                           (unsigned long)app_context->sts40_device.config.io_retry_delay_ms,
                           app_context->poll_enabled ? "on" : "off",
                           (unsigned long)app_context->poll_period_ms,
                           (unsigned)app_context->poll_bar_width,
                           bar_min_int,
                           bar_max_int);
        }
        else
        {
            Console_Printf(console,
                           "sensor=%s addr=%s rep=%s crc=%s retry=%u/%lu poll=%s %lu ms bar=%u\r\n",
                           app_context->sts40_ready ? "ready" : "not-ready",
                           address_list,
                           App_RepeatabilityToString(app_context->sts40_device.config.default_repeatability),
                           app_context->sts40_device.config.enable_crc_check ? "on" : "off",
                           app_context->sts40_device.config.io_retry_count,
                           (unsigned long)app_context->sts40_device.config.io_retry_delay_ms,
                           app_context->poll_enabled ? "on" : "off",
                           (unsigned long)app_context->poll_period_ms,
                           (unsigned)app_context->poll_bar_width);
        }
    }
}

static void App_CmdRead(Console *console, void *user_context, uint8_t argc, char **argv)
{
    AppConsoleContext *app_context = (AppConsoleContext *)user_context;

    if ((app_context == NULL) || !app_context->sts40_ready)
    {
        Console_WriteLine(console, "ERR sensor not ready");
        return;
    }

    if (argc < 2U)
    {
        Console_WriteLine(console, "Usage: read c|f|raw");
        return;
    }

    if (strcmp(argv[1], "raw") == 0)
    {
        uint8_t i;
        for (i = 0U; i < app_context->sensor_count; ++i)
        {
            uint16_t raw = 0U;
            uint8_t address = app_context->sensor_addresses[i];

            if (App_ReadRawForAddress(app_context, address, &raw) == STS40_STATUS_OK)
            {
                Console_Printf(console, "raw[0x%02X]=%u\r\n", address, raw);
            }
            else
            {
                Console_Printf(console, "ERR read raw addr=0x%02X\r\n", address);
            }
        }
        return;
    }

    if (strcmp(argv[1], "c") == 0)
    {
        uint8_t i;
        for (i = 0U; i < app_context->sensor_count; ++i)
        {
            float temperature_c = 0.0f;
            uint8_t address = app_context->sensor_addresses[i];

            if (App_ReadTemperatureForAddress(app_context, address, &temperature_c) == STS40_STATUS_OK)
            {
                int32_t temp_int = (int32_t)temperature_c;
                int32_t temp_frac = (int32_t)((temperature_c - temp_int) * 100.0f);
                if (temp_frac < 0) temp_frac = -temp_frac;
                Console_Printf(console, "temp_c[0x%02X]=%ld.%02ld\r\n", address, temp_int, temp_frac);
            }
            else
            {
                Console_Printf(console, "ERR read c addr=0x%02X\r\n", address);
            }
        }
        return;
    }

    if (strcmp(argv[1], "f") == 0)
    {
        uint8_t i;
        for (i = 0U; i < app_context->sensor_count; ++i)
        {
            float temperature_c = 0.0f;
            uint8_t address = app_context->sensor_addresses[i];

            if (App_ReadTemperatureForAddress(app_context, address, &temperature_c) == STS40_STATUS_OK)
            {
                float temperature_f = (temperature_c * 9.0f / 5.0f) + 32.0f;
                int32_t temp_int = (int32_t)temperature_f;
                int32_t temp_frac = (int32_t)((temperature_f - temp_int) * 100.0f);
                if (temp_frac < 0) temp_frac = -temp_frac;
                Console_Printf(console, "temp_f[0x%02X]=%ld.%02ld\r\n", address, temp_int, temp_frac);
            }
            else
            {
                Console_Printf(console, "ERR read f addr=0x%02X\r\n", address);
            }
        }
        return;
    }

    Console_WriteLine(console, "Usage: read c|f|raw");
}

static void App_CmdDebug(Console *console, void *user_context, uint8_t argc, char **argv)
{
    AppConsoleContext *app_context = (AppConsoleContext *)user_context;
    uint8_t command;
    uint8_t data[3];
    uint16_t i;

    (void)argc;
    (void)argv;

    if ((app_context == NULL) || !app_context->sts40_ready)
    {
        Console_WriteLine(console, "ERR sensor not ready");
        return;
    }

    command = 0xFDU; // HIGH repeatability measure
    
    if (App_Sts40_I2cWrite(Board_I2cHandle(), app_context->sensor_addresses[0], &command, 1U) != 0)
    {
        Console_WriteLine(console, "ERR write failed");
        return;
    }

    app_context->sts40_device.io.delay_ms(app_context->sts40_device.io.context, 9U);

    if (App_Sts40_I2cRead(Board_I2cHandle(), app_context->sensor_addresses[0], data, 3U) != 0)
    {
        Console_WriteLine(console, "ERR read failed");
        return;
    }

    Console_Printf(console, "raw bytes: ");
    for (i = 0U; i < 3U; ++i)
    {
        Console_Printf(console, "%02X ", data[i]);
    }
    Console_WriteLine(console, "");
    
    Console_Printf(console, "temp_raw=0x%04X (%u)\r\n", 
                   (uint16_t)((uint16_t)data[0] << 8U) | data[1],
                   (uint16_t)((uint16_t)data[0] << 8U) | data[1]);
}

static void App_CmdSerial(Console *console, void *user_context, uint8_t argc, char **argv)
{
    AppConsoleContext *app_context = (AppConsoleContext *)user_context;
    uint32_t serial_number = 0U;

    (void)argc;
    (void)argv;

    if ((app_context == NULL) || !app_context->sts40_ready)
    {
        Console_WriteLine(console, "ERR sensor not ready");
        return;
    }

    {
        uint8_t i;
        for (i = 0U; i < app_context->sensor_count; ++i)
        {
            uint8_t address = app_context->sensor_addresses[i];
            if (App_ReadSerialForAddress(app_context, address, &serial_number) == STS40_STATUS_OK)
            {
                Console_Printf(console, "serial[0x%02X]=0x%08lX\r\n", address, (unsigned long)serial_number);
            }
            else
            {
                Console_Printf(console, "ERR serial addr=0x%02X\r\n", address);
            }
        }
    }
}

static void App_CmdReset(Console *console, void *user_context, uint8_t argc, char **argv)
{
    AppConsoleContext *app_context = (AppConsoleContext *)user_context;
    Sts40Status status = STS40_STATUS_INVALID_ARG;

    if ((app_context == NULL) || !app_context->sts40_ready)
    {
        Console_WriteLine(console, "ERR sensor not ready");
        return;
    }

    if (argc < 2U)
    {
        Console_WriteLine(console, "Usage: reset soft|gc");
        return;
    }

    if (strcmp(argv[1], "soft") == 0)
    {
        status = Sts40_SoftReset(&app_context->sts40_device);
    }
    else if (strcmp(argv[1], "gc") == 0)
    {
        status = Sts40_GeneralCallReset(&app_context->sts40_device);
    }

    Console_WriteLine(console, (status == STS40_STATUS_OK) ? "OK" : "ERR reset");
}

static void App_CmdConfig(Console *console, void *user_context, uint8_t argc, char **argv)
{
    AppConsoleContext *app_context = (AppConsoleContext *)user_context;

    if ((app_context == NULL) || !app_context->sts40_ready)
    {
        Console_WriteLine(console, "ERR sensor not ready");
        return;
    }

    if ((argc >= 2U) && (strcmp(argv[1], "show") == 0))
    {
        App_CmdStatus(console, user_context, argc, argv);
        return;
    }

    if ((argc >= 3U) && (strcmp(argv[1], "addr") == 0))
    {
        uint8_t addresses[3] = {0U, 0U, 0U};
        uint8_t count = 0U;

        if (!App_ParseAddressList(argv[2], addresses, &count))
        {
            Console_WriteLine(console, "ERR address: 0x44|0x45|0x46|all|0x44,0x45...");
            return;
        }

        memcpy(app_context->sensor_addresses, addresses, sizeof(addresses));
        app_context->sensor_count = count;
        app_context->sts40_device.config.i2c_address = app_context->sensor_addresses[0];
        Console_WriteLine(console, "OK");
        return;
    }

    if ((argc >= 3U) && (strcmp(argv[1], "rep") == 0))
    {
        Sts40Repeatability repeatability;
        if (!App_ParseRepeatability(argv[2], &repeatability))
        {
            Console_WriteLine(console, "ERR rep low|medium|high");
            return;
        }
        app_context->sts40_device.config.default_repeatability = repeatability;
        Console_WriteLine(console, "OK");
        return;
    }

    if ((argc >= 3U) && (strcmp(argv[1], "crc") == 0))
    {
        if (strcmp(argv[2], "on") == 0)
        {
            app_context->sts40_device.config.enable_crc_check = true;
            Console_WriteLine(console, "OK");
            return;
        }
        if (strcmp(argv[2], "off") == 0)
        {
            app_context->sts40_device.config.enable_crc_check = false;
            Console_WriteLine(console, "OK");
            return;
        }

        Console_WriteLine(console, "ERR crc on|off");
        return;
    }

    if ((argc >= 4U) && (strcmp(argv[1], "retry") == 0))
    {
        uint32_t retry_count = 0U;
        uint32_t retry_delay_ms = 0U;

        if (!App_ParseU32(argv[2], &retry_count) || !App_ParseU32(argv[3], &retry_delay_ms))
        {
            Console_WriteLine(console, "ERR retry <count> <delay_ms>");
            return;
        }

        app_context->sts40_device.config.io_retry_count = (uint8_t)retry_count;
        app_context->sts40_device.config.io_retry_delay_ms = retry_delay_ms;
        Console_WriteLine(console, "OK");
        return;
    }

    Console_WriteLine(console, "Usage: config show|addr|rep|crc|retry");
}

static void App_CmdPoll(Console *console, void *user_context, uint8_t argc, char **argv)
{
    AppConsoleContext *app_context = (AppConsoleContext *)user_context;

    if (app_context == NULL)
    {
        return;
    }

    if (argc < 2U)
    {
        Console_WriteLine(console, "Usage: poll on <period_ms> [bar <min>,<width>,<max>]|off|bar <min>,<width>,<max>");
        return;
    }

    if (strcmp(argv[1], "off") == 0)
    {
        app_context->poll_enabled = false;
        Console_WriteLine(console, "OK");
        return;
    }

    if ((strcmp(argv[1], "bar") == 0) && (argc >= 3U))
    {
        uint32_t bar_min = 0U;
        uint32_t bar_width = 0U;
        uint32_t bar_max = 0U;

        if (!App_ParseBarConfig(argv[2], &bar_min, &bar_width, &bar_max))
        {
            Console_WriteLine(console, "ERR bar format: <min>,<width>,<max>");
            return;
        }

        if (bar_width == 0U || bar_width > 40U)
        {
            Console_WriteLine(console, "ERR bar width 1-40");
            return;
        }

        if (bar_max <= bar_min)
        {
            Console_WriteLine(console, "ERR bar max > min");
            return;
        }

        app_context->poll_bar_width = (uint8_t)bar_width;
        app_context->poll_bar_min = (float)bar_min;
        app_context->poll_bar_max = (float)bar_max;
        Console_WriteLine(console, "OK");
        return;
    }

    if ((strcmp(argv[1], "on") == 0) && (argc >= 3U))
    {
        uint32_t period_ms = 0U;
        if (!App_ParseU32(argv[2], &period_ms) || (period_ms == 0U))
        {
            Console_WriteLine(console, "ERR period_ms > 0");
            return;
        }

        app_context->poll_period_ms = period_ms;
        app_context->poll_enabled = true;
        app_context->next_poll_at_ms = HAL_GetTick() + app_context->poll_period_ms;

        if ((argc >= 5U) && (strcmp(argv[3], "bar") == 0))
        {
            uint32_t bar_min = 0U;
            uint32_t bar_width = 0U;
            uint32_t bar_max = 0U;

            if (!App_ParseBarConfig(argv[4], &bar_min, &bar_width, &bar_max))
            {
                Console_WriteLine(console, "ERR bar format: <min>,<width>,<max>");
                return;
            }

            if (bar_width == 0U || bar_width > 40U)
            {
                Console_WriteLine(console, "ERR bar width 1-40");
                return;
            }

            if (bar_max <= bar_min)
            {
                Console_WriteLine(console, "ERR bar max > min");
                return;
            }

            app_context->poll_bar_width = (uint8_t)bar_width;
            app_context->poll_bar_min = (float)bar_min;
            app_context->poll_bar_max = (float)bar_max;
        }

        Console_WriteLine(console, "OK");
        return;
    }

    Console_WriteLine(console, "Usage: poll on <period_ms> [bar <min>,<width>,<max>]|off|bar <min>,<width>,<max>");
}

static void App_CmdLed(Console *console, void *user_context, uint8_t argc, char **argv)
{
    (void)user_context;

    if (argc < 2U)
    {
        Console_WriteLine(console, "Usage: led on|off|toggle");
        return;
    }

    if (strcmp(argv[1], "on") == 0)
    {
        LedDriver_On();
        Console_WriteLine(console, "OK");
        return;
    }

    if (strcmp(argv[1], "off") == 0)
    {
        LedDriver_Off();
        Console_WriteLine(console, "OK");
        return;
    }

    if (strcmp(argv[1], "toggle") == 0)
    {
        LedDriver_Toggle();
        Console_WriteLine(console, "OK");
        return;
    }

    Console_WriteLine(console, "Usage: led on|off|toggle");
}

static const ConsoleCommand app_console_commands[] = {
    {"help", "Show command list", App_CmdHelp},
    {"status", "Show sensor/runtime status", App_CmdStatus},
    {"read", "Read sensor value", App_CmdRead},
    {"debug", "Debug: dump raw I2C bytes", App_CmdDebug},
    {"serial", "Read sensor serial number", App_CmdSerial},
    {"reset", "Reset sensor", App_CmdReset},
    {"config", "Configure sensor", App_CmdConfig},
    {"poll", "Enable/disable periodic polling", App_CmdPoll},
    {"led", "Control LED", App_CmdLed},
};

void App_Blinky_Run(void)
{
    AppConsoleContext app_context;
    Sts40Io sts40_io;
    Sts40Config sts40_config;
    Console console;
    ConsoleInit console_init;
    uint32_t now_ms;

    memset(&app_context, 0, sizeof(app_context));

    Board_Init();
    MX_USB_DEVICE_Init();
    LedDriver_Init();

    sts40_io.context = Board_I2cHandle();
    sts40_io.i2c_write = App_Sts40_I2cWrite;
    sts40_io.i2c_read = App_Sts40_I2cRead;
    sts40_io.delay_ms = App_Sts40_DelayMs;

    Sts40_ConfigDefaults(&sts40_config);
    sts40_config.i2c_address = BOARD_STS40_I2C_ADDRESS;

    if ((Board_I2cInit() == HAL_OK) &&
        (Sts40_Init(&app_context.sts40_device, &sts40_io, &sts40_config) == STS40_STATUS_OK))
    {
        app_context.sts40_ready = true;
    }

    app_context.poll_enabled = true;
    app_context.poll_period_ms = BOARD_STS40_POLL_PERIOD_MS;
    app_context.poll_bar_width = 0U;
    app_context.poll_bar_min = 0.0f;
    app_context.poll_bar_max = 50.0f;
    app_context.sensor_addresses[0] = sts40_config.i2c_address;
    app_context.sensor_count = 1U;
    app_context.blink_period_ms = BOARD_BLINK_TOGGLE_PERIOD_MS;
    now_ms = HAL_GetTick();
    app_context.next_poll_at_ms = now_ms + app_context.poll_period_ms;
    app_context.next_blink_at_ms = now_ms + app_context.blink_period_ms;

    console_init.io_context = NULL;
    console_init.read_byte = App_ConsoleReadByte;
    console_init.write_bytes = App_ConsoleWrite;
    console_init.commands = app_console_commands;
    console_init.command_count = (uint8_t)(sizeof(app_console_commands) / sizeof(app_console_commands[0]));
    console_init.user_context = &app_context;
    (void)Console_Init(&console, &console_init);

    while (1)
    {
        now_ms = HAL_GetTick();
        Console_Process(&console);

        if (app_context.sts40_ready && app_context.poll_enabled &&
            ((int32_t)(now_ms - app_context.next_poll_at_ms) >= 0))
        {
            uint8_t i;

            for (i = 0U; i < app_context.sensor_count; ++i)
            {
                float temperature_c = 0.0f;
                uint8_t address = app_context.sensor_addresses[i];

                if (App_ReadTemperatureForAddress(&app_context, address, &temperature_c) == STS40_STATUS_OK)
                {
                    int32_t temp_int = (int32_t)temperature_c;
                    int32_t temp_frac = (int32_t)((temperature_c - temp_int) * 100.0f);
                    if (temp_frac < 0) temp_frac = -temp_frac;

                    Console_Printf(&console, "poll addr=0x%02X temp_c=%ld.%02ld", address, temp_int, temp_frac);

                    if (app_context.poll_bar_width > 0U)
                    {
                        char bar_buffer[128];
                        App_MakeTempBar(bar_buffer, sizeof(bar_buffer), temperature_c,
                                        app_context.poll_bar_width,
                                        app_context.poll_bar_min,
                                        app_context.poll_bar_max);
                        Console_Printf(&console, "%s", bar_buffer);
                    }

                    Console_Printf(&console, "\r\n");
                }
                else
                {
                    Console_Printf(&console, "poll addr=0x%02X ERR\r\n", address);
                }
            }

            app_context.next_poll_at_ms = now_ms + app_context.poll_period_ms;
        }

        if ((int32_t)(now_ms - app_context.next_blink_at_ms) >= 0)
        {
            LedDriver_Toggle();
            app_context.next_blink_at_ms = now_ms + app_context.blink_period_ms;
        }

        Board_DelayMs(1U);
    }
}
