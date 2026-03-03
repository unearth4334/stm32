#include "app_blinky.h"

#include "board.h"
#include "board_config.h"
#include "led_driver.h"
#include "sts40_driver.h"

static int32_t App_Sts40_I2cWrite(void *context, uint8_t address, const uint8_t *data, uint16_t length)
{
    I2C_HandleTypeDef *i2c_handle = (I2C_HandleTypeDef *)context;

    if ((i2c_handle == NULL) || (data == NULL))
    {
        return -1;
    }

    return (HAL_I2C_Master_Transmit(i2c_handle,
                                    (uint16_t)(address << 1U),
                                    (uint8_t *)data,
                                    length,
                                    BOARD_I2C_TIMEOUT_MS) == HAL_OK) ? 0 : -1;
}

static int32_t App_Sts40_I2cRead(void *context, uint8_t address, uint8_t *data, uint16_t length)
{
    I2C_HandleTypeDef *i2c_handle = (I2C_HandleTypeDef *)context;

    if ((i2c_handle == NULL) || (data == NULL))
    {
        return -1;
    }

    return (HAL_I2C_Master_Receive(i2c_handle,
                                   (uint16_t)(address << 1U),
                                   data,
                                   length,
                                   BOARD_I2C_TIMEOUT_MS) == HAL_OK) ? 0 : -1;
}

static void App_Sts40_DelayMs(void *context, uint32_t delay_ms)
{
    (void)context;
    Board_DelayMs(delay_ms);
}

void App_Blinky_Run(void)
{
    Sts40Device sts40_device;
    Sts40Io sts40_io;
    Sts40Config sts40_config;
    bool sts40_ready = false;
    uint32_t sensor_poll_elapsed_ms = 0U;

    Board_Init();
    LedDriver_Init();

    sts40_io.context = Board_I2cHandle();
    sts40_io.i2c_write = App_Sts40_I2cWrite;
    sts40_io.i2c_read = App_Sts40_I2cRead;
    sts40_io.delay_ms = App_Sts40_DelayMs;

    Sts40_ConfigDefaults(&sts40_config);
    sts40_config.i2c_address = BOARD_STS40_I2C_ADDRESS;

    if ((Board_I2cInit() == HAL_OK) &&
        (Sts40_Init(&sts40_device, &sts40_io, &sts40_config) == STS40_STATUS_OK))
    {
        sts40_ready = true;
    }

    while (1)
    {
        uint32_t blink_period_ms = BOARD_BLINK_TOGGLE_PERIOD_MS;

        if (sts40_ready && (sensor_poll_elapsed_ms >= BOARD_STS40_POLL_PERIOD_MS))
        {
            float temperature_c = 0.0f;

            if (Sts40_MeasureTemperatureC(&sts40_device,
                                          STS40_REPEATABILITY_HIGH,
                                          &temperature_c) == STS40_STATUS_OK)
            {
                if (temperature_c >= 30.0f)
                {
                    blink_period_ms = 200U;
                }
            }

            sensor_poll_elapsed_ms = 0U;
        }

        LedDriver_Toggle();
        Board_DelayMs(blink_period_ms);
        sensor_poll_elapsed_ms += blink_period_ms;
    }
}
