#include "ads7822_service.h"

#include "ads7822/ads7822_driver.h"

static ads7822_t s_ads7822;
static uint8_t s_ads7822_ready;

int ads7822_service_init(void)
{
    if (ads7822_init_default(&s_ads7822) != ADS7822_OK) {
        s_ads7822_ready = 0U;
        return -1;
    }

    s_ads7822_ready = 1U;
    return 0;
}

int ads7822_service_read_raw(uint16_t *sample)
{
    if ((s_ads7822_ready == 0U) || (sample == NULL)) {
        return -1;
    }

    ads7822_load_switch_enable(1U);
    return (ads7822_read_raw(&s_ads7822, sample) == ADS7822_OK) ? 0 : -1;
}

int ads7822_service_read_voltage_v(float *voltage_v)
{
    if ((s_ads7822_ready == 0U) || (voltage_v == NULL)) {
        return -1;
    }

    ads7822_load_switch_enable(1U);
    return (ads7822_read_voltage_v(&s_ads7822, voltage_v) == ADS7822_OK) ? 0 : -1;
}