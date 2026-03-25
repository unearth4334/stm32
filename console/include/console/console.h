#ifndef CONSOLE_CONSOLE_H
#define CONSOLE_CONSOLE_H

#include <stdint.h>

typedef enum {
    CONSOLE_LOG_ERROR = 0,
    CONSOLE_LOG_WARN = 1,
    CONSOLE_LOG_INFO = 2,
    CONSOLE_LOG_DEBUG = 3,
} console_log_level_t;

typedef struct {
    uint32_t signature;
    uint32_t tick_ms;
    uint32_t code;
    uint32_t info0;
    uint32_t info1;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t bfar;
    uint32_t mmfar;
    char reason[24];
} console_fault_record_t;

void console_init(void);
void console_poll(void);
void console_set_level(console_log_level_t level);
console_log_level_t console_get_level(void);

void console_log(console_log_level_t level,
                 const char *tag,
                 const char *fmt,
                 ...);

void console_log_panic(const char *tag, const char *fmt, ...);

void console_record_fault(const char *reason,
                         uint32_t code,
                         uint32_t info0,
                         uint32_t info1);

const console_fault_record_t *console_fault_last(void);

void console_assert_failed(const char *expr, const char *file, uint32_t line);

#define CONSOLE_LOGE(tag, fmt, ...) console_log(CONSOLE_LOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGW(tag, fmt, ...) console_log(CONSOLE_LOG_WARN, tag, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGI(tag, fmt, ...) console_log(CONSOLE_LOG_INFO, tag, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGD(tag, fmt, ...) console_log(CONSOLE_LOG_DEBUG, tag, fmt, ##__VA_ARGS__)

#endif /* CONSOLE_CONSOLE_H */