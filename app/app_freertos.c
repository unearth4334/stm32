#include "app_freertos.h"
#include "app_blinky.h"
#include "platform/i2c.h"
#include "osal/task.h"
#include "osal/delay.h"
#include "console/console.h"
#include "ina232_service.h"

#ifndef APP_BLINKY_TASK_STACK_WORDS
#define APP_BLINKY_TASK_STACK_WORDS 256U
#endif

#ifndef APP_BLINKY_TASK_PRIORITY
#define APP_BLINKY_TASK_PRIORITY 1U
#endif

#ifndef APP_CONSOLE_TASK_STACK_WORDS
#define APP_CONSOLE_TASK_STACK_WORDS 256U
#endif

#ifndef APP_CONSOLE_TASK_PRIORITY
#define APP_CONSOLE_TASK_PRIORITY 1U
#endif

#ifndef APP_CONSOLE_POLL_PERIOD_MS
#define APP_CONSOLE_POLL_PERIOD_MS 10U
#endif

#ifndef APP_INA232_INIT_TASK_STACK_WORDS
#define APP_INA232_INIT_TASK_STACK_WORDS 256U
#endif

#ifndef APP_INA232_INIT_TASK_PRIORITY
#define APP_INA232_INIT_TASK_PRIORITY 1U
#endif

#ifndef APP_INA232_INIT_RETRY_MS
#define APP_INA232_INIT_RETRY_MS 1000U
#endif

static void app_blinky_task(void *arg)
{
    (void)arg;

    app_blinky_init();

    while (1)
    {
        app_blinky_run();
    }
}

static void app_console_task(void *arg)
{
    (void)arg;

    while (1)
    {
        /* console_poll() blocks internally on the RX stream buffer,
         * so no artificial delay is needed here. */
        console_poll();
    }
}

static void app_ina232_init_task(void *arg)
{
    platform_i2c_handle_t i2c = platform_i2c_primary_handle();
    uint8_t ready = 0U;

    (void)arg;

    while (1)
    {
        if (ready == 0U) {
            if (platform_i2c_init_primary(i2c, 100000U) == PLATFORM_I2C_OK) {
                if (ina232_service_init(i2c,
                                        INA232_SERVICE_DEFAULT_ADDR,
                                        INA232_SERVICE_DEFAULT_SHUNT_OHMS,
                                        INA232_SERVICE_DEFAULT_MAX_CURRENT_A) == INA232_OK) {
                    ready = 1U;
                }
            }
        }

        osal_delay_ms(APP_INA232_INIT_RETRY_MS);
    }
}

void app_rtos_init(void)
{
    const osal_task_config_t cfg = {
        .name = "blinky",
        .stack_words = APP_BLINKY_TASK_STACK_WORDS,
        .priority = APP_BLINKY_TASK_PRIORITY
    };

    const osal_task_config_t console_cfg = {
        .name = "console",
        .stack_words = APP_CONSOLE_TASK_STACK_WORDS,
        .priority = APP_CONSOLE_TASK_PRIORITY
    };

    const osal_task_config_t ina232_cfg = {
        .name = "ina232",
        .stack_words = APP_INA232_INIT_TASK_STACK_WORDS,
        .priority = APP_INA232_INIT_TASK_PRIORITY
    };

    (void)osal_task_create(app_blinky_task, 0, &cfg);
    (void)osal_task_create(app_console_task, 0, &console_cfg);
    (void)osal_task_create(app_ina232_init_task, 0, &ina232_cfg);
}
