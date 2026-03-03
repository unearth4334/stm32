#ifndef STS40_DRIVER_CONFIG_H
#define STS40_DRIVER_CONFIG_H

#define STS40_I2C_ADDRESS_DEFAULT                    (0x44U)
#define STS40_I2C_ADDRESS_ALT1                       (0x45U)
#define STS40_I2C_ADDRESS_ALT2                       (0x46U)

#define STS40_CMD_MEASURE_HIGH                       (0xFDU)
#define STS40_CMD_MEASURE_MEDIUM                     (0xF6U)
#define STS40_CMD_MEASURE_LOW                        (0xE0U)
#define STS40_CMD_READ_SERIAL                        (0x89U)
#define STS40_CMD_SOFT_RESET                         (0x94U)

#define STS40_GENERAL_CALL_ADDRESS                   (0x00U)
#define STS40_GENERAL_CALL_RESET_CMD                 (0x06U)

#define STS40_MEASUREMENT_WAIT_MS_LOW                (2U)
#define STS40_MEASUREMENT_WAIT_MS_MEDIUM             (5U)
#define STS40_MEASUREMENT_WAIT_MS_HIGH               (9U)
#define STS40_RESET_WAIT_MS                          (1U)

#define STS40_CRC8_POLYNOMIAL                        (0x31U)
#define STS40_CRC8_INIT                              (0xFFU)
#define STS40_CRC8_FINAL_XOR                         (0x00U)

#define STS40_DRIVER_ENABLE_CRC_CHECK                (0U)
#define STS40_DRIVER_ENABLE_RETRY                    (1U)
#define STS40_DRIVER_IO_RETRY_COUNT                  (2U)
#define STS40_DRIVER_IO_RETRY_DELAY_MS               (1U)

#define STS40_DRIVER_DEFAULT_REPEATABILITY           (2U)

#define STS40_DRIVER_TEMP_SCALE_NUMERATOR            (175.0f)
#define STS40_DRIVER_TEMP_SCALE_DENOMINATOR          (65535.0f)
#define STS40_DRIVER_TEMP_OFFSET_C                   (-45.0f)

#define STS40_DRIVER_TEMP_F_SCALE_NUMERATOR          (315.0f)
#define STS40_DRIVER_TEMP_F_SCALE_DENOMINATOR        (65535.0f)
#define STS40_DRIVER_TEMP_F_OFFSET                   (-49.0f)

#endif
