#include "app_blinky.h"

#include "board.h"
#include "board_config.h"
#include "console.h"
#include "led_driver.h"
#include "sts40_driver.h"
#include "usb_device.h"
#include "usb_console_port.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    Sts40Device sts40_device;
    bool sts40_ready;
    bool poll_enabled;
    uint32_t poll_period_ms;
    uint32_t next_poll_at_ms;
    uint32_t next_blink_at_ms;
    uint32_t blink_period_ms;
} AppConsoleContext;

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
    Console_WriteLine(console, "  config addr <0x44|0x45|0x46>");
    Console_WriteLine(console, "  config rep <low|medium|high>");
    Console_WriteLine(console, "  config crc <on|off>");
    Console_WriteLine(console, "  config retry <count> <delay_ms>");
    Console_WriteLine(console, "  poll on <period_ms>");
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

    Console_Printf(console,
                   "sensor=%s addr=0x%02X rep=%s crc=%s retry=%u/%lu poll=%s %lu ms\r\n",
                   app_context->sts40_ready ? "ready" : "not-ready",
                   app_context->sts40_device.config.i2c_address,
                   App_RepeatabilityToString(app_context->sts40_device.config.default_repeatability),
                   app_context->sts40_device.config.enable_crc_check ? "on" : "off",
                   app_context->sts40_device.config.io_retry_count,
                   (unsigned long)app_context->sts40_device.config.io_retry_delay_ms,
                   app_context->poll_enabled ? "on" : "off",
                   (unsigned long)app_context->poll_period_ms);
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
        uint16_t raw = 0U;
        if (Sts40_MeasureRaw(&app_context->sts40_device,
                             app_context->sts40_device.config.default_repeatability,
                             &raw) == STS40_STATUS_OK)
        {
            Console_Printf(console, "raw=%u\r\n", raw);
        }
        else
        {
            Console_WriteLine(console, "ERR read raw");
        }
        return;
    }

    if (strcmp(argv[1], "c") == 0)
    {
        float temperature_c = 0.0f;
        if (Sts40_MeasureTemperatureC(&app_context->sts40_device,
                                      app_context->sts40_device.config.default_repeatability,
                                      &temperature_c) == STS40_STATUS_OK)
        {
            Console_Printf(console, "temp_c=%.2f\r\n", temperature_c);
        }
        else
        {
            Console_WriteLine(console, "ERR read c");
        }
        return;
    }

    if (strcmp(argv[1], "f") == 0)
    {
        float temperature_f = 0.0f;
        if (Sts40_MeasureTemperatureF(&app_context->sts40_device,
                                      app_context->sts40_device.config.default_repeatability,
                                      &temperature_f) == STS40_STATUS_OK)
        {
            Console_Printf(console, "temp_f=%.2f\r\n", temperature_f);
        }
        else
        {
            Console_WriteLine(console, "ERR read f");
        }
        return;
    }

    Console_WriteLine(console, "Usage: read c|f|raw");
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

    if (Sts40_ReadSerial(&app_context->sts40_device, &serial_number) == STS40_STATUS_OK)
    {
        Console_Printf(console, "serial=0x%08lX\r\n", (unsigned long)serial_number);
    }
    else
    {
        Console_WriteLine(console, "ERR serial");
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
        uint32_t address = 0U;
        if (!App_ParseU32(argv[2], &address))
        {
            Console_WriteLine(console, "ERR invalid address");
            return;
        }

        if ((address != 0x44U) && (address != 0x45U) && (address != 0x46U))
        {
            Console_WriteLine(console, "ERR address must be 0x44/0x45/0x46");
            return;
        }

        app_context->sts40_device.config.i2c_address = (uint8_t)address;
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
        Console_WriteLine(console, "Usage: poll on <period_ms>|off");
        return;
    }

    if (strcmp(argv[1], "off") == 0)
    {
        app_context->poll_enabled = false;
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
        Console_WriteLine(console, "OK");
        return;
    }

    Console_WriteLine(console, "Usage: poll on <period_ms>|off");
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
            float temperature_c = 0.0f;

            if (Sts40_MeasureTemperatureC(&app_context.sts40_device,
                                          app_context.sts40_device.config.default_repeatability,
                                          &temperature_c) == STS40_STATUS_OK)
            {
                Console_Printf(&console, "poll temp_c=%.2f\r\n", temperature_c);
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
