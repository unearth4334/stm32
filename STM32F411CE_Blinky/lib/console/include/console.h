#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

#include "console_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*ConsoleReadByte)(void *context, uint8_t *byte_out);
typedef int32_t (*ConsoleWriteBytes)(void *context, const uint8_t *data, uint16_t length);

typedef struct Console Console;

typedef struct
{
    const char *name;
    const char *help;
    void (*handler)(Console *console, void *user_context, uint8_t argc, char **argv);
} ConsoleCommand;

typedef struct
{
    void *io_context;
    ConsoleReadByte read_byte;
    ConsoleWriteBytes write_bytes;
    const ConsoleCommand *commands;
    uint8_t command_count;
    void *user_context;
} ConsoleInit;

struct Console
{
    void *io_context;
    ConsoleReadByte read_byte;
    ConsoleWriteBytes write_bytes;
    const ConsoleCommand *commands;
    uint8_t command_count;
    void *user_context;
    char line_buffer[CONSOLE_MAX_LINE_LENGTH];
    uint16_t line_length;
};

bool Console_Init(Console *console, const ConsoleInit *init);
void Console_Process(Console *console);

void Console_WriteString(Console *console, const char *text);
void Console_WriteLine(Console *console, const char *text);
void Console_Printf(Console *console, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
