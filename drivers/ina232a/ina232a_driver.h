#ifndef INA232A_DRIVER_H
#define INA232A_DRIVER_H

/**
 * @file ina232a_driver.h
 * @brief Portable INA232A current/power monitor driver.
 *
 * Driver layer only depends on the platform I2C abstraction.
 */

#include <stdbool.h>
#include <stdint.h>

#include "platform/i2c.h"

/* ---------- I2C base/default address and A1/A0 map ----------------------- */
#define INA232A_ADDR_GND_GND    0x40U
#define INA232A_ADDR_GND_VS     0x41U
#define INA232A_ADDR_GND_SDA    0x42U
#define INA232A_ADDR_GND_SCL    0x43U
#define INA232A_ADDR_VS_GND     0x44U
#define INA232A_ADDR_VS_VS      0x45U
#define INA232A_ADDR_VS_SDA     0x46U
#define INA232A_ADDR_VS_SCL     0x47U
#define INA232A_ADDR_SDA_GND    0x48U
#define INA232A_ADDR_SDA_VS     0x49U
#define INA232A_ADDR_SDA_SDA    0x4AU
#define INA232A_ADDR_SDA_SCL    0x4BU
#define INA232A_ADDR_SCL_GND    0x4CU
#define INA232A_ADDR_SCL_VS     0x4DU
#define INA232A_ADDR_SCL_SDA    0x4EU
#define INA232A_ADDR_SCL_SCL    0x4FU

/* ---------- Register map -------------------------------------------------- */
#define INA232A_REG_CONFIG          0x00U
#define INA232A_REG_SHUNT_VOLTAGE   0x01U
#define INA232A_REG_BUS_VOLTAGE     0x02U
#define INA232A_REG_POWER           0x03U
#define INA232A_REG_CURRENT         0x04U
#define INA232A_REG_CALIBRATION     0x05U
#define INA232A_REG_MASK_ENABLE     0x06U
#define INA232A_REG_ALERT_LIMIT     0x07U
#define INA232A_REG_MFR_ID          0xFEU
#define INA232A_REG_DIE_ID          0xFFU

/* ---------- Known ID values ---------------------------------------------- */
#define INA232A_MFR_ID_TI           0x5449U

/* ---------- Config register field masks/shifts --------------------------- */
#define INA232A_CFG_RST_MASK        0x8000U

#define INA232A_CFG_AVG_SHIFT       9U
#define INA232A_CFG_AVG_MASK        (0x7U << INA232A_CFG_AVG_SHIFT)

#define INA232A_CFG_VBUSCT_SHIFT    6U
#define INA232A_CFG_VBUSCT_MASK     (0x7U << INA232A_CFG_VBUSCT_SHIFT)

#define INA232A_CFG_VSHCT_SHIFT     3U
#define INA232A_CFG_VSHCT_MASK      (0x7U << INA232A_CFG_VSHCT_SHIFT)

#define INA232A_CFG_MODE_SHIFT      0U
#define INA232A_CFG_MODE_MASK       (0x7U << INA232A_CFG_MODE_SHIFT)

/* ---------- Mask/Enable register bits ------------------------------------ */
#define INA232A_ME_SOL              0x8000U
#define INA232A_ME_SUL              0x4000U
#define INA232A_ME_BOL              0x2000U
#define INA232A_ME_BUL              0x1000U
#define INA232A_ME_POL              0x0800U
#define INA232A_ME_CNVR             0x0400U
#define INA232A_ME_AFF              0x0200U
#define INA232A_ME_CVRF             0x0100U
#define INA232A_ME_OVF              0x0080U
#define INA232A_ME_APOL             0x0002U
#define INA232A_ME_LEN              0x0001U

/* ---------- LSB sizes ----------------------------------------------------- */
#define INA232A_SHUNT_LSB_UV        2.5f
#define INA232A_BUS_LSB_MV          1.25f
#define INA232A_POWER_LSB_CURRENT_MULT 25.0f

/* ---------- Configuration enums ------------------------------------------ */
typedef enum {
    INA232A_AVG_1    = 0,
    INA232A_AVG_4    = 1,
    INA232A_AVG_16   = 2,
    INA232A_AVG_64   = 3,
    INA232A_AVG_128  = 4,
    INA232A_AVG_256  = 5,
    INA232A_AVG_512  = 6,
    INA232A_AVG_1024 = 7,
} ina232a_avg_t;

typedef enum {
    INA232A_CONV_TIME_140US  = 0,
    INA232A_CONV_TIME_204US  = 1,
    INA232A_CONV_TIME_332US  = 2,
    INA232A_CONV_TIME_588US  = 3,
    INA232A_CONV_TIME_1100US = 4,
    INA232A_CONV_TIME_2116US = 5,
    INA232A_CONV_TIME_4156US = 6,
    INA232A_CONV_TIME_8244US = 7,
} ina232a_conv_time_t;

typedef enum {
    INA232A_MODE_POWER_DOWN          = 0,
    INA232A_MODE_SHUNT_TRIG          = 1,
    INA232A_MODE_BUS_TRIG            = 2,
    INA232A_MODE_SHUNT_BUS_TRIG      = 3,
    INA232A_MODE_ADC_OFF             = 4,
    INA232A_MODE_SHUNT_CONT          = 5,
    INA232A_MODE_BUS_CONT            = 6,
    INA232A_MODE_SHUNT_BUS_CONT      = 7,
} ina232a_mode_t;

typedef enum {
    INA232A_ALERT_SHUNT_OVER = 0,
    INA232A_ALERT_SHUNT_UNDER,
    INA232A_ALERT_BUS_OVER,
    INA232A_ALERT_BUS_UNDER,
    INA232A_ALERT_POWER_OVER,
    INA232A_ALERT_CONV_READY,
} ina232a_alert_func_t;

typedef enum {
    INA232A_OK = 0,
    INA232A_ERR_INVALID_ARG = -1,
    INA232A_ERR_I2C = -2,
    INA232A_ERR_NO_DEVICE = -3,
    INA232A_ERR_TIMEOUT = -4,
} ina232a_status_t;

typedef struct {
    platform_i2c_handle_t i2c;
    uint8_t dev_addr_7bit;
    float shunt_ohms;
    float current_lsb_a;
    uint16_t cal_reg;
} ina232a_t;

typedef struct {
    ina232a_avg_t avg;
    ina232a_conv_time_t bus_conv_time;
    ina232a_conv_time_t shunt_conv_time;
    ina232a_mode_t mode;
} ina232a_config_t;

int ina232a_init(ina232a_t *dev,
                 platform_i2c_handle_t i2c,
                 uint8_t dev_addr_7bit,
                 float shunt_ohms,
                 float max_expected_current_a);

int ina232a_reset(ina232a_t *dev);

int ina232a_configure(ina232a_t *dev, const ina232a_config_t *cfg);

int ina232a_read_shunt_voltage_mv(ina232a_t *dev, float *shunt_mv);

int ina232a_read_bus_voltage_v(ina232a_t *dev, float *bus_v);

int ina232a_read_current_a(ina232a_t *dev, float *current_a);

int ina232a_read_power_w(ina232a_t *dev, float *power_w);

int ina232a_is_conversion_ready(ina232a_t *dev, bool *ready);

int ina232a_configure_alert(ina232a_t *dev,
                            ina232a_alert_func_t alert,
                            bool latch_enable,
                            bool active_high,
                            float limit);

int ina232a_read_alert_flags(ina232a_t *dev, uint16_t *flags);

int ina232a_read_ids(ina232a_t *dev, uint16_t *mfr_id, uint16_t *die_id);

int ina232a_write_reg(ina232a_t *dev, uint8_t reg, uint16_t value);

int ina232a_read_reg(ina232a_t *dev, uint8_t reg, uint16_t *value);

#endif /* INA232A_DRIVER_H */
