#include "ina232_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INA232_SVC_MSG_LEN 96U

static ina232_t s_ina232;
static uint8_t s_ina232_ready;

static void ina232_service_set_msg(char *out, uint16_t out_len, const char *msg)
{
    if ((out == NULL) || (out_len == 0U)) {
        return;
    }

    (void)snprintf(out, out_len, "%s", msg);
}

static int ina232_service_parse_u16(const char *value, uint16_t *parsed)
{
    unsigned long v;
    char *end = NULL;

    if ((value == NULL) || (parsed == NULL)) {
        return -1;
    }

    v = strtoul(value, &end, 0);
    if ((end == value) || (*end != '\0') || (v > 0xFFFFUL)) {
        return -1;
    }

    *parsed = (uint16_t)v;
    return 0;
}

int ina232_service_init(platform_i2c_handle_t i2c,
                        uint8_t dev_addr_7bit,
                        float shunt_ohms,
                        float max_expected_current_a)
{
    ina232_config_t cfg = {
        .avg = INA232_AVG_16,
        .bus_conv_time = INA232_CONV_TIME_1100US,
        .shunt_conv_time = INA232_CONV_TIME_1100US,
        .mode = INA232_MODE_SHUNT_BUS_CONT,
    };
    int st;

    st = ina232_init(&s_ina232, i2c, dev_addr_7bit, shunt_ohms, max_expected_current_a);
    if (st != INA232_OK) {
        s_ina232_ready = 0U;
        return st;
    }

    st = ina232_configure(&s_ina232, &cfg);
    if (st != INA232_OK) {
        s_ina232_ready = 0U;
        return st;
    }

    s_ina232_ready = 1U;
    return INA232_OK;
}

int ina232_service_read_field(const char *field, char *out, uint16_t out_len)
{
    float fv;
    long scaled;
    platform_i2c_bus_guard_status_t bus_guard;
    uint16_t reg;
    uint16_t mfr_id;
    uint16_t die_id;
    int st;

    if ((field == NULL) || (out == NULL) || (out_len == 0U)) {
        return INA232_ERR_INVALID_ARG;
    }

    if (strcmp(field, "bus_guard") == 0) {
        st = platform_i2c_primary_bus_guard_status(&bus_guard);
        if (st != PLATFORM_I2C_OK) {
            ina232_service_set_msg(out, out_len, "bus_guard read failed");
            return INA232_ERR_I2C;
        }

        (void)snprintf(out,
                       out_len,
                       "bus_guard=%s window=%s conf=%u n=%u start=%lu stop=%lu rs=%lu sr=%lu sf=%lu dr=%lu df=%lu int=%lu jit=%lu span=%lu scl=%u sda=%u",
                       (bus_guard.bus_idle != 0U) ? "idle" : "busy",
                       (bus_guard.in_predicted_window != 0U) ? "blocked" : "open",
                       (unsigned int)bus_guard.predictor_confident,
                       (unsigned int)bus_guard.predictor_samples,
                       (unsigned long)bus_guard.start_count,
                       (unsigned long)bus_guard.stop_count,
                       (unsigned long)bus_guard.repeated_start_count,
                       (unsigned long)bus_guard.scl_rise_count,
                       (unsigned long)bus_guard.scl_fall_count,
                       (unsigned long)bus_guard.sda_rise_count,
                       (unsigned long)bus_guard.sda_fall_count,
                       (unsigned long)bus_guard.interval_ms,
                       (unsigned long)bus_guard.jitter_ms,
                       (unsigned long)bus_guard.transaction_span_ms,
                       (unsigned int)bus_guard.scl_high,
                       (unsigned int)bus_guard.sda_high);
        return INA232_OK;
    }

    if (!s_ina232_ready) {
        ina232_service_set_msg(out, out_len, "ina232 not initialized");
        return INA232_ERR_NO_DEVICE;
    }

    if (strcmp(field, "die_id") == 0) {
        st = ina232_read_ids(&s_ina232, NULL, &die_id);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "die_id read failed");
            return st;
        }
        (void)snprintf(out, out_len, "die_id=0x%04X", die_id);
        return INA232_OK;
    }

    if (strcmp(field, "mfr_id") == 0) {
        st = ina232_read_ids(&s_ina232, &mfr_id, NULL);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "mfr_id read failed");
            return st;
        }
        (void)snprintf(out, out_len, "mfr_id=0x%04X", mfr_id);
        return INA232_OK;
    }

    if (strcmp(field, "shunt_voltage") == 0) {
        st = ina232_read_shunt_voltage_mv(&s_ina232, &fv);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "shunt_voltage read failed");
            return st;
        }
        scaled = (long)(fv * 1000.0f);
        (void)snprintf(out, out_len, "shunt_voltage=%ld.%03ld mV",
                       scaled / 1000L,
                       labs(scaled % 1000L));
        return INA232_OK;
    }

    if (strcmp(field, "bus_voltage") == 0) {
        st = ina232_read_bus_voltage_v(&s_ina232, &fv);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "bus_voltage read failed");
            return st;
        }
        scaled = (long)(fv * 1000.0f);
        (void)snprintf(out, out_len, "bus_voltage=%ld.%03ld V",
                       scaled / 1000L,
                       labs(scaled % 1000L));
        return INA232_OK;
    }

    if (strcmp(field, "current") == 0) {
        st = ina232_read_current_a(&s_ina232, &fv);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "current read failed");
            return st;
        }
        scaled = (long)(fv * 1000000.0f);
        (void)snprintf(out, out_len, "current=%ld.%06ld A",
                       scaled / 1000000L,
                       labs(scaled % 1000000L));
        return INA232_OK;
    }

    if (strcmp(field, "power") == 0) {
        st = ina232_read_power_w(&s_ina232, &fv);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "power read failed");
            return st;
        }
        scaled = (long)(fv * 1000000.0f);
        (void)snprintf(out, out_len, "power=%ld.%06ld W",
                       scaled / 1000000L,
                       labs(scaled % 1000000L));
        return INA232_OK;
    }

    if (strcmp(field, "config") == 0) {
        st = ina232_read_reg(&s_ina232, INA232_REG_CONFIG, &reg);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "config read failed");
            return st;
        }
        (void)snprintf(out, out_len, "config=0x%04X", reg);
        return INA232_OK;
    }

    if (strcmp(field, "calibration") == 0) {
        st = ina232_read_reg(&s_ina232, INA232_REG_CALIBRATION, &reg);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "calibration read failed");
            return st;
        }
        (void)snprintf(out, out_len, "calibration=0x%04X", reg);
        return INA232_OK;
    }

    if (strcmp(field, "alert_limit") == 0) {
        st = ina232_read_reg(&s_ina232, INA232_REG_ALERT_LIMIT, &reg);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "alert_limit read failed");
            return st;
        }
        (void)snprintf(out, out_len, "alert_limit=0x%04X", reg);
        return INA232_OK;
    }

    if (strcmp(field, "mask_enable") == 0) {
        st = ina232_read_reg(&s_ina232, INA232_REG_MASK_ENABLE, &reg);
        if (st != INA232_OK) {
            ina232_service_set_msg(out, out_len, "mask_enable read failed");
            return st;
        }
        (void)snprintf(out, out_len, "mask_enable=0x%04X", reg);
        return INA232_OK;
    }

    (void)snprintf(out, out_len, "unknown field: %s", field);
    return INA232_ERR_INVALID_ARG;
}

int ina232_service_write_field(const char *field,
                               const char *value,
                               char *out,
                               uint16_t out_len)
{
    uint16_t reg_addr;
    uint16_t reg_value;
    int st;

    if ((field == NULL) || (value == NULL) || (out == NULL) || (out_len == 0U)) {
        return INA232_ERR_INVALID_ARG;
    }

    if (!s_ina232_ready) {
        ina232_service_set_msg(out, out_len, "ina232 not initialized");
        return INA232_ERR_NO_DEVICE;
    }

    if (ina232_service_parse_u16(value, &reg_value) != 0) {
        ina232_service_set_msg(out, out_len, "invalid value, use decimal or 0xHEX");
        return INA232_ERR_INVALID_ARG;
    }

    if (strcmp(field, "config") == 0) {
        reg_addr = INA232_REG_CONFIG;
    } else if (strcmp(field, "calibration") == 0) {
        reg_addr = INA232_REG_CALIBRATION;
    } else if (strcmp(field, "alert_limit") == 0) {
        reg_addr = INA232_REG_ALERT_LIMIT;
    } else if (strcmp(field, "mask_enable") == 0) {
        reg_addr = INA232_REG_MASK_ENABLE;
    } else if ((strcmp(field, "die_id") == 0) ||
               (strcmp(field, "bus_guard") == 0) ||
               (strcmp(field, "mfr_id") == 0) ||
               (strcmp(field, "shunt_voltage") == 0) ||
               (strcmp(field, "bus_voltage") == 0) ||
               (strcmp(field, "current") == 0) ||
               (strcmp(field, "power") == 0)) {
        (void)snprintf(out, out_len, "field '%s' is read-only", field);
        return INA232_ERR_INVALID_ARG;
    } else {
        (void)snprintf(out, out_len, "unknown field: %s", field);
        return INA232_ERR_INVALID_ARG;
    }

    st = ina232_write_reg(&s_ina232, (uint8_t)reg_addr, reg_value);
    if (st != INA232_OK) {
        (void)snprintf(out, out_len, "write failed: %s", field);
        return st;
    }

    (void)snprintf(out, out_len, "%s=0x%04X", field, reg_value);
    return INA232_OK;
}
