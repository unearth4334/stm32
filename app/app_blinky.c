#include "app_blinky.h"
#include "ads7822_service.h"
#include "console/console.h"
#include "led_service.h"
#include "osal/delay.h"

/* Blink period defined at compile-time via app_config.h if present */
#ifndef BLINK_PERIOD_MS
#define BLINK_PERIOD_MS 500U
#endif

#define ADS7822_LOG_PERIOD_LOOPS 8U

static uint32_t s_ads7822_log_divider;

void app_blinky_init(void)
{
    led_service_init();
    if (ads7822_service_init() != 0) {
        CONSOLE_LOGW("ads7822", "init failed");
    }
}

void app_blinky_run(void)
{
    uint16_t sample;

    led_service_toggle();

    s_ads7822_log_divider++;
    if (s_ads7822_log_divider >= ADS7822_LOG_PERIOD_LOOPS) {
        s_ads7822_log_divider = 0U;
        if (ads7822_service_read_raw(&sample) == 0) {
            CONSOLE_LOGI("ads7822", "sample=%u", (unsigned int)sample);
        } else {
            CONSOLE_LOGW("ads7822", "read failed");
        }
    }

    osal_delay_ms(BLINK_PERIOD_MS);
}
