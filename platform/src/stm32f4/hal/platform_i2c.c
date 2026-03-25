/**
 * @file platform_i2c.c
 * @brief STM32F4 HAL implementation of the platform I2C abstraction.
 */

#include "platform/i2c.h"

#include <limits.h>
#include <stddef.h>

#include "board.h"
#include "stm32f4xx_hal.h"

#define PLATFORM_I2C_PRIMARY_IDLE_GUARD_MS  3U
#define PLATFORM_I2C_PREDICTOR_INTERVAL_HISTORY  8U
#define PLATFORM_I2C_PREDICTOR_DURATION_HISTORY  8U
#define PLATFORM_I2C_PREDICTOR_MIN_SAMPLES       3U
#define PLATFORM_I2C_PREDICTOR_MAX_JITTER_MS     250U
#define PLATFORM_I2C_PREDICTOR_MIN_JITTER_MS     10U
#define PLATFORM_I2C_PREDICTOR_STALE_FACTOR      2U
#define PLATFORM_I2C_PREDICTOR_PRE_WINDOW_MS     5U
#define PLATFORM_I2C_PREDICTOR_BURST_GAP_MS      60U

static I2C_HandleTypeDef s_i2c1;
static uint8_t s_i2c1_ready;
static uint8_t s_i2c1_guard_ready;
static volatile uint8_t s_i2c1_lock;
static volatile uint8_t s_i2c1_local_tx_active;
static volatile uint32_t s_i2c1_last_activity_ms;
static uint8_t s_i2c1_prev_scl_high;
static uint8_t s_i2c1_prev_sda_high;
static uint8_t s_i2c1_external_tx_active;
static uint8_t s_i2c1_burst_active;
static uint32_t s_i2c1_burst_count;
static uint32_t s_i2c1_burst_start_ms;
static uint32_t s_i2c1_burst_last_event_ms;
static uint32_t s_i2c1_last_burst_start_ms;
static uint32_t s_i2c1_start_count;
static uint32_t s_i2c1_stop_count;
static uint32_t s_i2c1_repeated_start_count;
static uint32_t s_i2c1_scl_rise_count;
static uint32_t s_i2c1_scl_fall_count;
static uint32_t s_i2c1_sda_rise_count;
static uint32_t s_i2c1_sda_fall_count;
static uint32_t s_i2c1_last_scl_edge_ms;
static uint32_t s_i2c1_last_sda_edge_ms;
static uint32_t s_i2c1_external_tx_start_ms;
static uint32_t s_i2c1_last_external_start_ms;
static uint32_t s_i2c1_last_external_stop_ms;
static uint32_t s_i2c1_interval_history[PLATFORM_I2C_PREDICTOR_INTERVAL_HISTORY];
static uint32_t s_i2c1_duration_history[PLATFORM_I2C_PREDICTOR_DURATION_HISTORY];
static uint8_t s_i2c1_interval_index;
static uint8_t s_i2c1_interval_count;
static uint8_t s_i2c1_duration_index;
static uint8_t s_i2c1_duration_count;
static uint32_t s_i2c1_interval_estimate_ms;
static uint32_t s_i2c1_jitter_estimate_ms;
static uint32_t s_i2c1_duration_estimate_ms;
static uint8_t s_i2c1_predictor_confident;

static void platform_i2c_predictor_push_interval(uint32_t interval_ms);
static void platform_i2c_predictor_push_duration(uint32_t duration_ms);
static void platform_i2c_predictor_refresh(void);

static void platform_i2c_predictor_note_burst(uint32_t burst_start_ms, uint32_t burst_end_ms)
{
    if (burst_end_ms <= burst_start_ms) {
        return;
    }

    if ((s_i2c1_last_burst_start_ms != 0U) && (burst_start_ms > s_i2c1_last_burst_start_ms)) {
        platform_i2c_predictor_push_interval(burst_start_ms - s_i2c1_last_burst_start_ms);
    }

    platform_i2c_predictor_push_duration(burst_end_ms - burst_start_ms);
    s_i2c1_last_burst_start_ms = burst_start_ms;
    s_i2c1_burst_count++;
    platform_i2c_predictor_refresh();
}

static void platform_i2c_predictor_maybe_close_burst(uint32_t now_ms)
{
    if ((s_i2c1_burst_active == 0U) || ((now_ms - s_i2c1_burst_last_event_ms) < PLATFORM_I2C_PREDICTOR_BURST_GAP_MS)) {
        return;
    }

    platform_i2c_predictor_note_burst(s_i2c1_burst_start_ms, s_i2c1_burst_last_event_ms);
    s_i2c1_burst_active = 0U;
}

static void platform_i2c_predictor_note_external_activity(uint32_t now_ms)
{
    if ((s_i2c1_burst_active != 0U) && ((now_ms - s_i2c1_burst_last_event_ms) >= PLATFORM_I2C_PREDICTOR_BURST_GAP_MS)) {
        platform_i2c_predictor_note_burst(s_i2c1_burst_start_ms, s_i2c1_burst_last_event_ms);
        s_i2c1_burst_active = 0U;
    }

    if (s_i2c1_burst_active == 0U) {
        s_i2c1_burst_start_ms = now_ms;
        s_i2c1_burst_active = 1U;
    }

    s_i2c1_burst_last_event_ms = now_ms;
}

static void platform_i2c_predictor_sort(uint32_t *values, uint8_t count)
{
    uint8_t i;

    for (i = 1U; i < count; ++i) {
        uint32_t key = values[i];
        uint8_t j = i;

        while ((j > 0U) && (values[j - 1U] > key)) {
            values[j] = values[j - 1U];
            --j;
        }
        values[j] = key;
    }
}

static uint32_t platform_i2c_predictor_median(const uint32_t *values, uint8_t count)
{
    uint32_t scratch[PLATFORM_I2C_PREDICTOR_INTERVAL_HISTORY];
    uint8_t i;

    if ((values == NULL) || (count == 0U)) {
        return 0U;
    }

    for (i = 0U; i < count; ++i) {
        scratch[i] = values[i];
    }

    platform_i2c_predictor_sort(scratch, count);
    return scratch[count / 2U];
}

static void platform_i2c_predictor_push_interval(uint32_t interval_ms)
{
    s_i2c1_interval_history[s_i2c1_interval_index] = interval_ms;
    s_i2c1_interval_index = (uint8_t)((s_i2c1_interval_index + 1U) % PLATFORM_I2C_PREDICTOR_INTERVAL_HISTORY);
    if (s_i2c1_interval_count < PLATFORM_I2C_PREDICTOR_INTERVAL_HISTORY) {
        s_i2c1_interval_count++;
    }
}

static void platform_i2c_predictor_push_duration(uint32_t duration_ms)
{
    s_i2c1_duration_history[s_i2c1_duration_index] = duration_ms;
    s_i2c1_duration_index = (uint8_t)((s_i2c1_duration_index + 1U) % PLATFORM_I2C_PREDICTOR_DURATION_HISTORY);
    if (s_i2c1_duration_count < PLATFORM_I2C_PREDICTOR_DURATION_HISTORY) {
        s_i2c1_duration_count++;
    }
}

static void platform_i2c_predictor_refresh(void)
{
    uint32_t duration_estimate;
    uint32_t max_delta = 0U;
    uint32_t estimate = 0U;
    uint8_t i;

    if (s_i2c1_interval_count == 0U) {
        s_i2c1_interval_estimate_ms = 0U;
        s_i2c1_jitter_estimate_ms = PLATFORM_I2C_PREDICTOR_MAX_JITTER_MS;
        s_i2c1_predictor_confident = 0U;
    } else {
        estimate = platform_i2c_predictor_median(s_i2c1_interval_history, s_i2c1_interval_count);
        for (i = 0U; i < s_i2c1_interval_count; ++i) {
            uint32_t sample = s_i2c1_interval_history[i];
            uint32_t delta = (sample > estimate) ? (sample - estimate) : (estimate - sample);

            if (delta > max_delta) {
                max_delta = delta;
            }
        }

        if (max_delta < PLATFORM_I2C_PREDICTOR_MIN_JITTER_MS) {
            max_delta = PLATFORM_I2C_PREDICTOR_MIN_JITTER_MS;
        }
        if (max_delta > PLATFORM_I2C_PREDICTOR_MAX_JITTER_MS) {
            max_delta = PLATFORM_I2C_PREDICTOR_MAX_JITTER_MS;
        }

        s_i2c1_interval_estimate_ms = estimate;
        s_i2c1_jitter_estimate_ms = max_delta;
        s_i2c1_predictor_confident = ((s_i2c1_interval_count >= PLATFORM_I2C_PREDICTOR_MIN_SAMPLES) &&
                                      (s_i2c1_stop_count >= PLATFORM_I2C_PREDICTOR_MIN_SAMPLES) &&
                                      (max_delta < (s_i2c1_interval_estimate_ms / 4U))) ? 1U : 0U;
    }

    if (s_i2c1_duration_count == 0U) {
        s_i2c1_duration_estimate_ms = PLATFORM_I2C_PRIMARY_IDLE_GUARD_MS;
    } else {
        duration_estimate = platform_i2c_predictor_median(s_i2c1_duration_history, s_i2c1_duration_count);
        s_i2c1_duration_estimate_ms = duration_estimate;
        if (s_i2c1_duration_estimate_ms < PLATFORM_I2C_PRIMARY_IDLE_GUARD_MS) {
            s_i2c1_duration_estimate_ms = PLATFORM_I2C_PRIMARY_IDLE_GUARD_MS;
        }
    }
}

static void platform_i2c_predictor_note_start(uint32_t now_ms)
{
    s_i2c1_start_count++;
    s_i2c1_last_external_start_ms = now_ms;
    s_i2c1_external_tx_start_ms = now_ms;
    s_i2c1_external_tx_active = 1U;
}

static void platform_i2c_predictor_note_stop(uint32_t now_ms)
{
    s_i2c1_stop_count++;
    s_i2c1_last_external_stop_ms = now_ms;
    s_i2c1_external_tx_active = 0U;
}

static uint8_t platform_i2c_primary_predictor_stale(uint32_t now_ms)
{
    if ((s_i2c1_predictor_confident == 0U) || (s_i2c1_interval_estimate_ms == 0U) || (s_i2c1_last_burst_start_ms == 0U)) {
        return 1U;
    }

    return ((now_ms - s_i2c1_last_burst_start_ms) >
            ((PLATFORM_I2C_PREDICTOR_STALE_FACTOR * s_i2c1_interval_estimate_ms) + s_i2c1_jitter_estimate_ms)) ? 1U : 0U;
}

static uint8_t platform_i2c_primary_in_predicted_window(uint32_t now_ms,
                                                         uint32_t *window_open_ms,
                                                         uint32_t *window_close_ms)
{
    uint32_t predicted_start_ms;
    uint32_t open_ms;
    uint32_t close_ms;

    if (window_open_ms != NULL) {
        *window_open_ms = 0U;
    }
    if (window_close_ms != NULL) {
        *window_close_ms = 0U;
    }

    if (platform_i2c_primary_predictor_stale(now_ms) != 0U) {
        s_i2c1_predictor_confident = 0U;
        return 0U;
    }

    predicted_start_ms = s_i2c1_last_burst_start_ms + s_i2c1_interval_estimate_ms;
    open_ms = predicted_start_ms - ((predicted_start_ms > (s_i2c1_jitter_estimate_ms + PLATFORM_I2C_PREDICTOR_PRE_WINDOW_MS)) ?
                                    (s_i2c1_jitter_estimate_ms + PLATFORM_I2C_PREDICTOR_PRE_WINDOW_MS) : predicted_start_ms);
    close_ms = predicted_start_ms + s_i2c1_duration_estimate_ms + s_i2c1_jitter_estimate_ms;

    if (window_open_ms != NULL) {
        *window_open_ms = open_ms;
    }
    if (window_close_ms != NULL) {
        *window_close_ms = close_ms;
    }

    return ((now_ms >= open_ms) && (now_ms <= close_ms)) ? 1U : 0U;
}

static void platform_i2c_primary_monitor_lines(uint8_t *scl_high, uint8_t *sda_high)
{
    if (scl_high != NULL) {
        *scl_high = (HAL_GPIO_ReadPin(BOARD_I2C1_MON_PORT, BOARD_I2C1_MON_SCL_PIN) == GPIO_PIN_SET) ? 1U : 0U;
    }

    if (sda_high != NULL) {
        *sda_high = (HAL_GPIO_ReadPin(BOARD_I2C1_MON_PORT, BOARD_I2C1_MON_SDA_PIN) == GPIO_PIN_SET) ? 1U : 0U;
    }
}

static uint8_t platform_i2c_primary_bus_idle_now(void)
{
    uint8_t scl_high;
    uint8_t sda_high;

    platform_i2c_primary_monitor_lines(&scl_high, &sda_high);

    if ((scl_high == 0U) || (sda_high == 0U)) {
        return 0U;
    }

    return ((HAL_GetTick() - s_i2c1_last_activity_ms) >= PLATFORM_I2C_PRIMARY_IDLE_GUARD_MS) ? 1U : 0U;
}

static void platform_i2c_primary_monitor_event(void)
{
    uint32_t now_ms = HAL_GetTick();
    uint8_t scl_high;
    uint8_t sda_high;
    uint8_t start_detected;
    uint8_t stop_detected;

    platform_i2c_primary_monitor_lines(&scl_high, &sda_high);
    s_i2c1_last_activity_ms = now_ms;
    platform_i2c_predictor_maybe_close_burst(now_ms);

    if ((s_i2c1_prev_scl_high == 0U) && (scl_high != 0U)) {
        s_i2c1_scl_rise_count++;
        s_i2c1_last_scl_edge_ms = now_ms;
    } else if ((s_i2c1_prev_scl_high != 0U) && (scl_high == 0U)) {
        s_i2c1_scl_fall_count++;
        s_i2c1_last_scl_edge_ms = now_ms;
    }

    if ((s_i2c1_prev_sda_high == 0U) && (sda_high != 0U)) {
        s_i2c1_sda_rise_count++;
        s_i2c1_last_sda_edge_ms = now_ms;
    } else if ((s_i2c1_prev_sda_high != 0U) && (sda_high == 0U)) {
        s_i2c1_sda_fall_count++;
        s_i2c1_last_sda_edge_ms = now_ms;
    }

    start_detected = ((s_i2c1_prev_sda_high != 0U) && (sda_high == 0U) && (scl_high != 0U)) ? 1U : 0U;
    stop_detected = ((s_i2c1_prev_sda_high == 0U) && (sda_high != 0U) && (scl_high != 0U)) ? 1U : 0U;

    if (s_i2c1_local_tx_active == 0U) {
        if ((start_detected != 0U) && (s_i2c1_external_tx_active == 0U)) {
            platform_i2c_predictor_note_external_activity(now_ms);
            platform_i2c_predictor_note_start(now_ms);
        } else if ((start_detected != 0U) && (s_i2c1_external_tx_active != 0U)) {
            platform_i2c_predictor_note_external_activity(now_ms);
            s_i2c1_repeated_start_count++;
        } else if ((stop_detected != 0U) && (s_i2c1_external_tx_active != 0U)) {
            platform_i2c_predictor_note_external_activity(now_ms);
            platform_i2c_predictor_note_stop(now_ms);
        }
    }

    s_i2c1_prev_scl_high = scl_high;
    s_i2c1_prev_sda_high = sda_high;
}

static int platform_i2c_primary_lock_acquire(uint32_t timeout_ms)
{
    uint32_t start_ms = HAL_GetTick();

    for (;;) {
        uint32_t primask = __get_PRIMASK();

        __disable_irq();
        if (s_i2c1_lock == 0U) {
            s_i2c1_lock = 1U;
            if (primask == 0U) {
                __enable_irq();
            }
            return PLATFORM_I2C_OK;
        }

        if (primask == 0U) {
            __enable_irq();
        }

        if ((HAL_GetTick() - start_ms) >= timeout_ms) {
            return PLATFORM_I2C_ERR_BUS;
        }

        HAL_Delay(1U);
    }
}

static void platform_i2c_primary_lock_release(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    s_i2c1_lock = 0U;
    if (primask == 0U) {
        __enable_irq();
    }
}

static int platform_i2c_primary_wait_for_idle(uint32_t timeout_ms)
{
    uint32_t start_ms = HAL_GetTick();

    if (s_i2c1_guard_ready == 0U) {
        return PLATFORM_I2C_OK;
    }

    while ((platform_i2c_primary_bus_idle_now() == 0U) ||
           (platform_i2c_primary_in_predicted_window(HAL_GetTick(), NULL, NULL) != 0U)) {
        if ((HAL_GetTick() - start_ms) >= timeout_ms) {
            return PLATFORM_I2C_ERR_BUS;
        }

        HAL_Delay(1U);
    }

    return PLATFORM_I2C_OK;
}

static int platform_i2c_primary_guard_init(void)
{
    GPIO_InitTypeDef gpio_cfg = {0};

    if (s_i2c1_guard_ready != 0U) {
        return PLATFORM_I2C_OK;
    }

    BOARD_I2C1_MON_GPIO_CLK_EN();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    gpio_cfg.Pin = BOARD_I2C1_MON_SCL_PIN | BOARD_I2C1_MON_SDA_PIN;
    gpio_cfg.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_cfg.Pull = GPIO_PULLUP;
    gpio_cfg.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BOARD_I2C1_MON_PORT, &gpio_cfg);

    s_i2c1_last_activity_ms = HAL_GetTick();
    platform_i2c_primary_monitor_lines(&s_i2c1_prev_scl_high, &s_i2c1_prev_sda_high);
    s_i2c1_guard_ready = 1U;

    HAL_NVIC_SetPriority(BOARD_I2C1_MON_IRQn, BOARD_I2C1_MON_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(BOARD_I2C1_MON_IRQn);

    return PLATFORM_I2C_OK;
}

int platform_i2c_primary_monitor_init(void)
{
    return platform_i2c_primary_guard_init();
}

platform_i2c_handle_t platform_i2c_primary_handle(void)
{
    return (platform_i2c_handle_t)&s_i2c1;
}

int platform_i2c_init_primary(platform_i2c_handle_t handle, uint32_t clock_hz)
{
    GPIO_InitTypeDef gpio_cfg = {0};

    if ((handle != (platform_i2c_handle_t)&s_i2c1) || (clock_hz == 0U)) {
        return PLATFORM_I2C_ERR_INVALID_ARG;
    }

    BOARD_I2C1_GPIO_CLK_EN();
    BOARD_I2C1_CLK_EN();

    gpio_cfg.Pin = BOARD_I2C1_SCL_PIN | BOARD_I2C1_SDA_PIN;
    gpio_cfg.Mode = GPIO_MODE_AF_OD;
    gpio_cfg.Pull = GPIO_PULLUP;
    gpio_cfg.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_cfg.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init((GPIO_TypeDef *)BOARD_I2C1_SCL_PORT, &gpio_cfg);

    s_i2c1.Instance = BOARD_I2C1_INSTANCE;
    s_i2c1.Init.ClockSpeed = clock_hz;
    s_i2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    s_i2c1.Init.OwnAddress1 = 0U;
    s_i2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    s_i2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    s_i2c1.Init.OwnAddress2 = 0U;
    s_i2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_i2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&s_i2c1) != HAL_OK) {
        s_i2c1_ready = 0U;
        return PLATFORM_I2C_ERR_BUS;
    }

    (void)platform_i2c_primary_guard_init();
    s_i2c1_ready = 1U;
    return PLATFORM_I2C_OK;
}

int platform_i2c_primary_bus_guard_status(platform_i2c_bus_guard_status_t *status)
{
    uint32_t now_ms;

    if (status == NULL) {
        return PLATFORM_I2C_ERR_INVALID_ARG;
    }

    if (s_i2c1_guard_ready == 0U) {
        if (platform_i2c_primary_guard_init() != PLATFORM_I2C_OK) {
            return PLATFORM_I2C_ERR_BUS;
        }
    }

    now_ms = HAL_GetTick();
    platform_i2c_predictor_maybe_close_burst(now_ms);
    status->ready = s_i2c1_guard_ready;
    status->idle_guard_ms = PLATFORM_I2C_PRIMARY_IDLE_GUARD_MS;
    status->last_activity_ms = s_i2c1_last_activity_ms;
    status->last_start_ms = s_i2c1_last_external_start_ms;
    status->last_stop_ms = s_i2c1_last_external_stop_ms;
    status->last_burst_start_ms = s_i2c1_last_burst_start_ms;
    status->burst_gap_ms = PLATFORM_I2C_PREDICTOR_BURST_GAP_MS;
    status->last_scl_edge_ms = s_i2c1_last_scl_edge_ms;
    status->last_sda_edge_ms = s_i2c1_last_sda_edge_ms;
    platform_i2c_primary_monitor_lines(&status->scl_high, &status->sda_high);
    status->bus_idle = (s_i2c1_guard_ready != 0U) ? platform_i2c_primary_bus_idle_now() : 1U;
    status->transaction_active = s_i2c1_external_tx_active;
    status->burst_active = s_i2c1_burst_active;
    status->predictor_confident = s_i2c1_predictor_confident;
    status->predictor_samples = s_i2c1_interval_count;
    status->burst_count = s_i2c1_burst_count;
    status->start_count = s_i2c1_start_count;
    status->stop_count = s_i2c1_stop_count;
    status->repeated_start_count = s_i2c1_repeated_start_count;
    status->scl_rise_count = s_i2c1_scl_rise_count;
    status->scl_fall_count = s_i2c1_scl_fall_count;
    status->sda_rise_count = s_i2c1_sda_rise_count;
    status->sda_fall_count = s_i2c1_sda_fall_count;
    status->interval_ms = s_i2c1_interval_estimate_ms;
    status->jitter_ms = s_i2c1_jitter_estimate_ms;
    status->transaction_span_ms = s_i2c1_duration_estimate_ms;
    status->in_predicted_window = platform_i2c_primary_in_predicted_window(now_ms,
                                                                           &status->next_window_open_ms,
                                                                           &status->next_window_close_ms);

    return PLATFORM_I2C_OK;
}

int platform_i2c_mem_write(platform_i2c_handle_t handle,
                           uint8_t dev_addr_7bit,
                           uint8_t reg,
                           const uint8_t *data,
                           uint16_t len,
                           uint32_t timeout_ms)
{
    HAL_StatusTypeDef st;
    I2C_HandleTypeDef *h;

    if ((handle == NULL) || ((data == NULL) && (len != 0U))) {
        return PLATFORM_I2C_ERR_INVALID_ARG;
    }

    if ((handle == (platform_i2c_handle_t)&s_i2c1) && (s_i2c1_ready == 0U)) {
        return PLATFORM_I2C_ERR_BUS;
    }

    h = (I2C_HandleTypeDef *)handle;
    if (handle == (platform_i2c_handle_t)&s_i2c1) {
        if (platform_i2c_primary_lock_acquire(timeout_ms) != PLATFORM_I2C_OK) {
            return PLATFORM_I2C_ERR_BUS;
        }

        if (platform_i2c_primary_wait_for_idle(timeout_ms) != PLATFORM_I2C_OK) {
            platform_i2c_primary_lock_release();
            return PLATFORM_I2C_ERR_BUS;
        }

        s_i2c1_local_tx_active = 1U;
        s_i2c1_last_activity_ms = HAL_GetTick();
    }

    st = HAL_I2C_Mem_Write(h,
                           (uint16_t)(dev_addr_7bit << 1),
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           (uint8_t *)data,
                           len,
                           timeout_ms);

    if (handle == (platform_i2c_handle_t)&s_i2c1) {
        s_i2c1_local_tx_active = 0U;
        s_i2c1_last_activity_ms = HAL_GetTick();
        platform_i2c_primary_lock_release();
    }

    return (st == HAL_OK) ? PLATFORM_I2C_OK : PLATFORM_I2C_ERR_BUS;
}

int platform_i2c_mem_read(platform_i2c_handle_t handle,
                          uint8_t dev_addr_7bit,
                          uint8_t reg,
                          uint8_t *data,
                          uint16_t len,
                          uint32_t timeout_ms)
{
    HAL_StatusTypeDef st;
    I2C_HandleTypeDef *h;

    if ((handle == NULL) || ((data == NULL) && (len != 0U))) {
        return PLATFORM_I2C_ERR_INVALID_ARG;
    }

    if ((handle == (platform_i2c_handle_t)&s_i2c1) && (s_i2c1_ready == 0U)) {
        return PLATFORM_I2C_ERR_BUS;
    }

    h = (I2C_HandleTypeDef *)handle;
    if (handle == (platform_i2c_handle_t)&s_i2c1) {
        if (platform_i2c_primary_lock_acquire(timeout_ms) != PLATFORM_I2C_OK) {
            return PLATFORM_I2C_ERR_BUS;
        }

        if (platform_i2c_primary_wait_for_idle(timeout_ms) != PLATFORM_I2C_OK) {
            platform_i2c_primary_lock_release();
            return PLATFORM_I2C_ERR_BUS;
        }

        s_i2c1_local_tx_active = 1U;
        s_i2c1_last_activity_ms = HAL_GetTick();
    }

    st = HAL_I2C_Mem_Read(h,
                          (uint16_t)(dev_addr_7bit << 1),
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          data,
                          len,
                          timeout_ms);

    if (handle == (platform_i2c_handle_t)&s_i2c1) {
        s_i2c1_local_tx_active = 0U;
        s_i2c1_last_activity_ms = HAL_GetTick();
        platform_i2c_primary_lock_release();
    }

    return (st == HAL_OK) ? PLATFORM_I2C_OK : PLATFORM_I2C_ERR_BUS;
}

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
    if ((gpio_pin == BOARD_I2C1_MON_SCL_PIN) || (gpio_pin == BOARD_I2C1_MON_SDA_PIN)) {
        platform_i2c_primary_monitor_event();
    }
}
