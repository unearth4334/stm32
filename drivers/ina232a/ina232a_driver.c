#include "ina232a/ina232a_driver.h"

#include <stddef.h>

#define INA232A_I2C_TIMEOUT_MS 50U
#define INA232A_RESET_POLL_MAX 20U

static int ina232a_i2c_write16(ina232a_t *dev, uint8_t reg, uint16_t value)
{
    uint8_t buf[2];

    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFFU);

    if (platform_i2c_mem_write(dev->i2c,
                               dev->dev_addr_7bit,
                               reg,
                               buf,
                               2U,
                               INA232A_I2C_TIMEOUT_MS) != PLATFORM_I2C_OK) {
        return INA232A_ERR_I2C;
    }

    return INA232A_OK;
}

static int ina232a_i2c_read16(ina232a_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];

    if (platform_i2c_mem_read(dev->i2c,
                              dev->dev_addr_7bit,
                              reg,
                              buf,
                              2U,
                              INA232A_I2C_TIMEOUT_MS) != PLATFORM_I2C_OK) {
        return INA232A_ERR_I2C;
    }

    *value = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
    return INA232A_OK;
}

static float ina232a_absf(float x)
{
    return (x < 0.0f) ? -x : x;
}

static uint16_t ina232a_alert_mask_for_func(ina232a_alert_func_t alert)
{
    switch (alert) {
        case INA232A_ALERT_SHUNT_OVER:
            return INA232A_ME_SOL;
        case INA232A_ALERT_SHUNT_UNDER:
            return INA232A_ME_SUL;
        case INA232A_ALERT_BUS_OVER:
            return INA232A_ME_BOL;
        case INA232A_ALERT_BUS_UNDER:
            return INA232A_ME_BUL;
        case INA232A_ALERT_POWER_OVER:
            return INA232A_ME_POL;
        case INA232A_ALERT_CONV_READY:
            return INA232A_ME_CNVR;
        default:
            return 0U;
    }
}

static uint16_t ina232a_limit_to_raw(const ina232a_t *dev,
                                     ina232a_alert_func_t alert,
                                     float limit)
{
    float raw = 0.0f;

    if (limit < 0.0f) {
        limit = 0.0f;
    }

    switch (alert) {
        case INA232A_ALERT_SHUNT_OVER:
        case INA232A_ALERT_SHUNT_UNDER:
            raw = (limit / 0.0025f);
            break;
        case INA232A_ALERT_BUS_OVER:
        case INA232A_ALERT_BUS_UNDER:
            raw = (limit / 0.00125f);
            break;
        case INA232A_ALERT_POWER_OVER:
            if (dev->current_lsb_a > 0.0f) {
                raw = limit / (INA232A_POWER_LSB_CURRENT_MULT * dev->current_lsb_a);
            }
            break;
        case INA232A_ALERT_CONV_READY:
            raw = 0.0f;
            break;
        default:
            raw = 0.0f;
            break;
    }

    if (raw > 65535.0f) {
        raw = 65535.0f;
    }

    return (uint16_t)raw;
}

int ina232a_write_reg(ina232a_t *dev, uint8_t reg, uint16_t value)
{
    if (dev == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    return ina232a_i2c_write16(dev, reg, value);
}

int ina232a_read_reg(ina232a_t *dev, uint8_t reg, uint16_t *value)
{
    if (dev == NULL || value == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    return ina232a_i2c_read16(dev, reg, value);
}

int ina232a_read_ids(ina232a_t *dev, uint16_t *mfr_id, uint16_t *die_id)
{
    int st;

    if (dev == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    if (mfr_id != NULL) {
        st = ina232a_read_reg(dev, INA232A_REG_MFR_ID, mfr_id);
        if (st != INA232A_OK) {
            return st;
        }
    }

    if (die_id != NULL) {
        st = ina232a_read_reg(dev, INA232A_REG_DIE_ID, die_id);
        if (st != INA232A_OK) {
            return st;
        }
    }

    return INA232A_OK;
}

int ina232a_reset(ina232a_t *dev)
{
    uint16_t cfg;
    uint32_t i;
    int st;

    if (dev == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    st = ina232a_read_reg(dev, INA232A_REG_CONFIG, &cfg);
    if (st != INA232A_OK) {
        return st;
    }

    st = ina232a_write_reg(dev, INA232A_REG_CONFIG, (uint16_t)(cfg | INA232A_CFG_RST_MASK));
    if (st != INA232A_OK) {
        return st;
    }

    for (i = 0U; i < INA232A_RESET_POLL_MAX; ++i) {
        st = ina232a_read_reg(dev, INA232A_REG_CONFIG, &cfg);
        if (st != INA232A_OK) {
            return st;
        }

        if ((cfg & INA232A_CFG_RST_MASK) == 0U) {
            return INA232A_OK;
        }
    }

    return INA232A_ERR_TIMEOUT;
}

int ina232a_configure(ina232a_t *dev, const ina232a_config_t *cfg)
{
    uint16_t reg = 0U;

    if (dev == NULL || cfg == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    if ((uint8_t)cfg->avg > 7U ||
        (uint8_t)cfg->bus_conv_time > 7U ||
        (uint8_t)cfg->shunt_conv_time > 7U ||
        (uint8_t)cfg->mode > 7U) {
        return INA232A_ERR_INVALID_ARG;
    }

    reg |= ((uint16_t)cfg->avg << INA232A_CFG_AVG_SHIFT) & INA232A_CFG_AVG_MASK;
    reg |= ((uint16_t)cfg->bus_conv_time << INA232A_CFG_VBUSCT_SHIFT) & INA232A_CFG_VBUSCT_MASK;
    reg |= ((uint16_t)cfg->shunt_conv_time << INA232A_CFG_VSHCT_SHIFT) & INA232A_CFG_VSHCT_MASK;
    reg |= ((uint16_t)cfg->mode << INA232A_CFG_MODE_SHIFT) & INA232A_CFG_MODE_MASK;

    return ina232a_write_reg(dev, INA232A_REG_CONFIG, reg);
}

int ina232a_init(ina232a_t *dev,
                 platform_i2c_handle_t i2c,
                 uint8_t dev_addr_7bit,
                 float shunt_ohms,
                 float max_expected_current_a)
{
    int st;
    float current_lsb;
    float cal_f;
    uint16_t mfr_id = 0U;

    if (dev == NULL || i2c == NULL || shunt_ohms <= 0.0f || max_expected_current_a <= 0.0f) {
        return INA232A_ERR_INVALID_ARG;
    }

    if (dev_addr_7bit > 0x7FU) {
        return INA232A_ERR_INVALID_ARG;
    }

    dev->i2c = i2c;
    dev->dev_addr_7bit = (uint8_t)(dev_addr_7bit & 0x7FU);
    dev->shunt_ohms = shunt_ohms;

    current_lsb = ina232a_absf(max_expected_current_a) / 32768.0f;
    if (current_lsb <= 0.0f) {
        return INA232A_ERR_INVALID_ARG;
    }

    dev->current_lsb_a = current_lsb;
    cal_f = 0.00512f / (dev->current_lsb_a * dev->shunt_ohms);
    if (cal_f < 1.0f) {
        cal_f = 1.0f;
    }
    if (cal_f > 65535.0f) {
        cal_f = 65535.0f;
    }

    dev->cal_reg = (uint16_t)cal_f;

    st = ina232a_reset(dev);
    if (st != INA232A_OK) {
        return st;
    }

    st = ina232a_write_reg(dev, INA232A_REG_CALIBRATION, dev->cal_reg);
    if (st != INA232A_OK) {
        return st;
    }

    st = ina232a_read_reg(dev, INA232A_REG_MFR_ID, &mfr_id);
    if (st != INA232A_OK) {
        return st;
    }

    if (mfr_id != INA232A_MFR_ID_TI) {
        return INA232A_ERR_NO_DEVICE;
    }

    return INA232A_OK;
}

int ina232a_read_shunt_voltage_mv(ina232a_t *dev, float *shunt_mv)
{
    uint16_t raw;
    int16_t sraw;
    int st;

    if (dev == NULL || shunt_mv == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    st = ina232a_read_reg(dev, INA232A_REG_SHUNT_VOLTAGE, &raw);
    if (st != INA232A_OK) {
        return st;
    }

    sraw = (int16_t)raw;
    *shunt_mv = (float)sraw * (INA232A_SHUNT_LSB_UV / 1000.0f);
    return INA232A_OK;
}

int ina232a_read_bus_voltage_v(ina232a_t *dev, float *bus_v)
{
    uint16_t raw;
    int st;

    if (dev == NULL || bus_v == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    st = ina232a_read_reg(dev, INA232A_REG_BUS_VOLTAGE, &raw);
    if (st != INA232A_OK) {
        return st;
    }

    *bus_v = ((float)raw * INA232A_BUS_LSB_MV) / 1000.0f;
    return INA232A_OK;
}

int ina232a_read_current_a(ina232a_t *dev, float *current_a)
{
    uint16_t raw;
    int16_t sraw;
    int st;

    if (dev == NULL || current_a == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    st = ina232a_read_reg(dev, INA232A_REG_CURRENT, &raw);
    if (st != INA232A_OK) {
        return st;
    }

    sraw = (int16_t)raw;
    *current_a = (float)sraw * dev->current_lsb_a;
    return INA232A_OK;
}

int ina232a_read_power_w(ina232a_t *dev, float *power_w)
{
    uint16_t raw;
    int st;

    if (dev == NULL || power_w == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    st = ina232a_read_reg(dev, INA232A_REG_POWER, &raw);
    if (st != INA232A_OK) {
        return st;
    }

    *power_w = (float)raw * (INA232A_POWER_LSB_CURRENT_MULT * dev->current_lsb_a);
    return INA232A_OK;
}

int ina232a_is_conversion_ready(ina232a_t *dev, bool *ready)
{
    uint16_t reg;
    int st;

    if (dev == NULL || ready == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    st = ina232a_read_reg(dev, INA232A_REG_MASK_ENABLE, &reg);
    if (st != INA232A_OK) {
        return st;
    }

    *ready = ((reg & INA232A_ME_CVRF) != 0U);
    return INA232A_OK;
}

int ina232a_read_alert_flags(ina232a_t *dev, uint16_t *flags)
{
    int st;

    if (dev == NULL || flags == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    st = ina232a_read_reg(dev, INA232A_REG_MASK_ENABLE, flags);
    if (st != INA232A_OK) {
        return st;
    }

    return INA232A_OK;
}

int ina232a_configure_alert(ina232a_t *dev,
                            ina232a_alert_func_t alert,
                            bool latch_enable,
                            bool active_high,
                            float limit)
{
    uint16_t mask = 0U;
    uint16_t limit_raw;
    int st;

    if (dev == NULL) {
        return INA232A_ERR_INVALID_ARG;
    }

    mask = ina232a_alert_mask_for_func(alert);
    if (mask == 0U) {
        return INA232A_ERR_INVALID_ARG;
    }

    if (latch_enable) {
        mask |= INA232A_ME_LEN;
    }
    if (active_high) {
        mask |= INA232A_ME_APOL;
    }

    st = ina232a_write_reg(dev, INA232A_REG_MASK_ENABLE, mask);
    if (st != INA232A_OK) {
        return st;
    }

    limit_raw = ina232a_limit_to_raw(dev, alert, limit);
    st = ina232a_write_reg(dev, INA232A_REG_ALERT_LIMIT, limit_raw);
    if (st != INA232A_OK) {
        return st;
    }

    return INA232A_OK;
}
