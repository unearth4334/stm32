#include "console.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static uint8_t Console_Tokenize(char *line, char **argv, uint8_t max_args)
{
    uint8_t argc = 0;
    char *cursor = line;

    while ((*cursor != '\0') && (argc < max_args))
    {
        while (*cursor == ' ')
        {
            ++cursor;
        }

        if (*cursor == '\0')
        {
            break;
        }

        argv[argc++] = cursor;

        while ((*cursor != '\0') && (*cursor != ' '))
        {
            ++cursor;
        }

        if (*cursor == '\0')
        {
            break;
        }

        *cursor = '\0';
        ++cursor;
    }

    return argc;
}

static void Console_WriteRaw(Console *console, const char *text, uint16_t length)
{
    if ((console == NULL) || (text == NULL) || (console->write_bytes == NULL))
    {
        return;
    }

    (void)console->write_bytes(console->io_context, (const uint8_t *)text, length);
}

static void Console_Prompt(Console *console)
{
    Console_WriteString(console, CONSOLE_PROMPT);
}

static void Console_DispatchLine(Console *console)
{
    char *argv[CONSOLE_MAX_ARGUMENTS];
    uint8_t argc;
    uint8_t index;

    if ((console == NULL) || (console->line_length == 0U))
    {
        return;
    }

    argc = Console_Tokenize(console->line_buffer, argv, CONSOLE_MAX_ARGUMENTS);
    if (argc == 0U)
    {
        return;
    }

    for (index = 0U; index < console->command_count; ++index)
    {
        if ((console->commands[index].name != NULL) &&
            (strcmp(argv[0], console->commands[index].name) == 0))
        {
            if (console->commands[index].handler != NULL)
            {
                console->commands[index].handler(console, console->user_context, argc, argv);
            }
            return;
        }
    }

    Console_Printf(console, "Unknown command: %s\r\n", argv[0]);
}

bool Console_Init(Console *console, const ConsoleInit *init)
{
    if ((console == NULL) || (init == NULL) || (init->read_byte == NULL) || (init->write_bytes == NULL))
    {
        return false;
    }

    memset(console, 0, sizeof(*console));

    console->io_context = init->io_context;
    console->read_byte = init->read_byte;
    console->write_bytes = init->write_bytes;
    console->commands = init->commands;
    console->command_count = init->command_count;
    console->user_context = init->user_context;

    Console_WriteString(console, CONSOLE_WELCOME_BANNER);
    Console_Prompt(console);

    return true;
}

void Console_Process(Console *console)
{
    uint8_t byte = 0U;

    if ((console == NULL) || (console->read_byte == NULL))
    {
        return;
    }

    while (console->read_byte(console->io_context, &byte) > 0)
    {
        if ((byte == '\r') || (byte == '\n'))
        {
#if CONSOLE_ENABLE_ECHO
            Console_WriteString(console, "\r\n");
#endif
            console->line_buffer[console->line_length] = '\0';
            Console_DispatchLine(console);
            console->line_length = 0U;
            console->line_buffer[0] = '\0';
            Console_Prompt(console);
            continue;
        }

#if CONSOLE_ENABLE_BACKSPACE
        if ((byte == '\b') || (byte == 0x7FU))
        {
            if (console->line_length > 0U)
            {
                --console->line_length;
                console->line_buffer[console->line_length] = '\0';
#if CONSOLE_ENABLE_ECHO
                Console_WriteString(console, "\b \b");
#endif
            }
            continue;
        }
#endif

        if ((byte >= 32U) && (byte <= 126U))
        {
            if (console->line_length < (CONSOLE_MAX_LINE_LENGTH - 1U))
            {
                console->line_buffer[console->line_length++] = (char)byte;
                console->line_buffer[console->line_length] = '\0';
#if CONSOLE_ENABLE_ECHO
                Console_WriteRaw(console, (const char *)&byte, 1U);
#endif
            }
        }
    }
}

void Console_WriteString(Console *console, const char *text)
{
    if (text == NULL)
    {
        return;
    }

    Console_WriteRaw(console, text, (uint16_t)strlen(text));
}

void Console_WriteLine(Console *console, const char *text)
{
    Console_WriteString(console, text);
    Console_WriteString(console, "\r\n");
}

void Console_Printf(Console *console, const char *format, ...)
{
    char buffer[CONSOLE_TX_BUFFER_LENGTH];
    va_list args;
    int written;

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (written <= 0)
    {
        return;
    }

    if (written >= (int)sizeof(buffer))
    {
        written = (int)sizeof(buffer) - 1;
    }

    Console_WriteRaw(console, buffer, (uint16_t)written);
}
