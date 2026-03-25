#include "console/console.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "platform/uart.h"
#include "stm32f4xx_hal.h"

#if defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "stream_buffer.h"
#endif

#define CONSOLE_LINE_MAX_LEN 96U
#define CONSOLE_MSG_MAX_LEN 192U
#define CONSOLE_FAULT_SIGNATURE 0x43464C54u /* CFLT */
#define CONSOLE_UART_BAUDRATE 115200U
#define CONSOLE_RX_STREAM_SIZE 64U

static uint8_t s_console_ready;
static console_log_level_t s_log_level = CONSOLE_LOG_INFO;
static platform_uart_handle_t s_uart = 0;
static char s_line_buf[CONSOLE_LINE_MAX_LEN];
static char s_last_line[CONSOLE_LINE_MAX_LEN];
static uint16_t s_line_len;
static uint32_t s_drop_count;
static console_fault_record_t s_fault_last;

#if defined(USE_FREERTOS)
static StreamBufferHandle_t s_rx_stream;

static void console_rx_irq_cb(uint8_t byte, void *ctx)
{
    BaseType_t higher_task_woken = pdFALSE;
    (void)ctx;
    if (s_rx_stream != NULL) {
        (void)xStreamBufferSendFromISR(s_rx_stream, &byte, 1U, &higher_task_woken);
        portYIELD_FROM_ISR(higher_task_woken);
    }
}
#endif

static void console_write_text(const char *text)
{
    if (text == NULL) {
        return;
    }

    if ((!s_console_ready) ||
        (platform_uart_write(s_uart, (const uint8_t *)text, (uint16_t)strlen(text), 10U) != PLATFORM_UART_OK)) {
        s_drop_count++;
    }
}

static const char *console_level_name(console_log_level_t level)
{
    switch (level) {
    case CONSOLE_LOG_ERROR:
        return "E";
    case CONSOLE_LOG_WARN:
        return "W";
    case CONSOLE_LOG_INFO:
        return "I";
    case CONSOLE_LOG_DEBUG:
        return "D";
    default:
        return "?";
    }
}

static void console_vlog(console_log_level_t level, const char *tag, const char *fmt, va_list args)
{
    char msg[CONSOLE_MSG_MAX_LEN];
    int written;

    if (level > s_log_level) {
        return;
    }

    if (tag == NULL) {
        tag = "sys";
    }

    written = snprintf(msg, sizeof(msg), "[%lu] %s/%s: ",
                       (unsigned long)HAL_GetTick(),
                       console_level_name(level),
                       tag);
    if ((written < 0) || ((size_t)written >= sizeof(msg))) {
        return;
    }

    (void)vsnprintf(&msg[written], sizeof(msg) - (size_t)written, fmt, args);
    (void)strncat(msg, "\r\n", sizeof(msg) - strlen(msg) - 1U);
    console_write_text(msg);
}

static void console_print_prompt(void)
{
    console_write_text("> ");
}

static void console_command_help(void)
{
    console_write_text("commands: help, version, uptime, log level <0-3>, fault last\r\n");
}

static void console_command_version(void)
{
    console_write_text("console v0.1\r\n");
}

static void console_command_uptime(void)
{
    char msg[64];
    (void)snprintf(msg, sizeof(msg), "uptime_ms=%lu\r\n", (unsigned long)HAL_GetTick());
    console_write_text(msg);
}

static void console_command_log_level(const char *arg)
{
    char msg[48];
    unsigned long level;

    if (arg == NULL) {
        (void)snprintf(msg, sizeof(msg), "log_level=%lu\r\n", (unsigned long)s_log_level);
        console_write_text(msg);
        return;
    }

    level = strtoul(arg, NULL, 10);
    if (level > (unsigned long)CONSOLE_LOG_DEBUG) {
        console_write_text("invalid level, expected 0..3\r\n");
        return;
    }

    s_log_level = (console_log_level_t)level;
    (void)snprintf(msg, sizeof(msg), "log_level=%lu\r\n", level);
    console_write_text(msg);
}

static void console_command_fault_last(void)
{
    char msg[160];

    if (s_fault_last.signature != CONSOLE_FAULT_SIGNATURE) {
        console_write_text("fault: none\r\n");
        return;
    }

    (void)snprintf(msg,
                   sizeof(msg),
                   "fault=%s code=0x%08lX info0=0x%08lX info1=0x%08lX cfsr=0x%08lX hfsr=0x%08lX\r\n",
                   s_fault_last.reason,
                   (unsigned long)s_fault_last.code,
                   (unsigned long)s_fault_last.info0,
                   (unsigned long)s_fault_last.info1,
                   (unsigned long)s_fault_last.cfsr,
                   (unsigned long)s_fault_last.hfsr);
    console_write_text(msg);
}

static void console_process_command(const char *line)
{
    if ((line == NULL) || (line[0] == '\0')) {
        return;
    }

    if (strcmp(line, "help") == 0) {
        console_command_help();
    } else if (strcmp(line, "version") == 0) {
        console_command_version();
    } else if (strcmp(line, "uptime") == 0) {
        console_command_uptime();
    } else if (strncmp(line, "log level", 9U) == 0) {
        const char *arg = NULL;

        if (line[9] == ' ') {
            arg = &line[10];
        }
        console_command_log_level(arg);
    } else if (strcmp(line, "fault last") == 0) {
        console_command_fault_last();
    } else {
        console_write_text("unknown command\r\n");
    }
}

void console_init(void)
{
    if (s_console_ready) {
        return;
    }

    s_uart = platform_uart_debug_handle();
    if (platform_uart_init(s_uart, CONSOLE_UART_BAUDRATE) != PLATFORM_UART_OK) {
        return;
    }

#if defined(USE_FREERTOS)
    s_rx_stream = xStreamBufferCreate(CONSOLE_RX_STREAM_SIZE, 1U);
    if (s_rx_stream == NULL) {
        return;
    }
    (void)platform_uart_set_rx_callback(s_uart, console_rx_irq_cb, NULL);
    (void)platform_uart_start_rx_it(s_uart);
#endif

    s_console_ready = 1U;
    console_write_text("console ready\r\n");
    console_print_prompt();
}

static void console_process_char(uint8_t ch)
{
    if ((ch == '\r') || (ch == '\n')) {
        console_write_text("\r\n");

        s_line_buf[s_line_len] = '\0';
        if (s_line_len == 0U) {
            if (s_last_line[0] != '\0') {
                console_process_command(s_last_line);
            }
        } else {
            (void)strncpy(s_last_line, s_line_buf, sizeof(s_last_line) - 1U);
            s_last_line[sizeof(s_last_line) - 1U] = '\0';
            console_process_command(s_line_buf);
        }

        s_line_len = 0U;
        console_print_prompt();
        return;
    }

    if ((ch == 0x08U) || (ch == 0x7FU)) {
        if (s_line_len > 0U) {
            s_line_len--;
        }
        return;
    }

    if ((ch >= 0x20U) && (ch <= 0x7EU) && (s_line_len < (CONSOLE_LINE_MAX_LEN - 1U))) {
        s_line_buf[s_line_len++] = (char)ch;
        (void)platform_uart_write(s_uart, &ch, 1U, 10U);
    }
}

void console_poll(void)
{
    uint8_t ch;

    if (!s_console_ready) {
        return;
    }

#if defined(USE_FREERTOS)
    /* Block until a byte arrives or 100 ms elapses */
    if (xStreamBufferReceive(s_rx_stream, &ch, 1U, pdMS_TO_TICKS(100U)) == 0U) {
        return;
    }
    console_process_char(ch);
    /* Drain any bytes already buffered without further blocking */
    while (xStreamBufferReceive(s_rx_stream, &ch, 1U, 0) > 0U) {
        console_process_char(ch);
    }
#else
    {
        uint16_t read_len;
        while (platform_uart_read(s_uart, &ch, 1U, &read_len, 0U) == PLATFORM_UART_OK) {
            if (read_len == 0U) {
                break;
            }
            console_process_char(ch);
        }
    }
#endif
}

void console_set_level(console_log_level_t level)
{
    if (level > CONSOLE_LOG_DEBUG) {
        return;
    }

    s_log_level = level;
}

console_log_level_t console_get_level(void)
{
    return s_log_level;
}

void console_log(console_log_level_t level,
                 const char *tag,
                 const char *fmt,
                 ...)
{
    va_list args;

    if (!s_console_ready) {
        console_init();
    }

    va_start(args, fmt);
    console_vlog(level, tag, fmt, args);
    va_end(args);
}

void console_log_panic(const char *tag, const char *fmt, ...)
{
    va_list args;

    if (!s_console_ready) {
        console_init();
    }

    va_start(args, fmt);
    console_vlog(CONSOLE_LOG_ERROR, tag, fmt, args);
    va_end(args);
}

void console_record_fault(const char *reason,
                         uint32_t code,
                         uint32_t info0,
                         uint32_t info1)
{
    (void)memset(&s_fault_last, 0, sizeof(s_fault_last));

    s_fault_last.signature = CONSOLE_FAULT_SIGNATURE;
    s_fault_last.tick_ms = HAL_GetTick();
    s_fault_last.code = code;
    s_fault_last.info0 = info0;
    s_fault_last.info1 = info1;
    s_fault_last.cfsr = SCB->CFSR;
    s_fault_last.hfsr = SCB->HFSR;
    s_fault_last.bfar = SCB->BFAR;
    s_fault_last.mmfar = SCB->MMFAR;

    if (reason != NULL) {
        (void)strncpy(s_fault_last.reason, reason, sizeof(s_fault_last.reason) - 1U);
    }
}

const console_fault_record_t *console_fault_last(void)
{
    return &s_fault_last;
}

void console_assert_failed(const char *expr, const char *file, uint32_t line)
{
    console_record_fault("assert", 0xA55E47U, (uint32_t)(uintptr_t)expr, line);
    console_log_panic("assert", "assert failed: %s @ %s:%lu", expr, file, (unsigned long)line);
}