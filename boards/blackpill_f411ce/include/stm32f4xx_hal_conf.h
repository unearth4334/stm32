#ifndef STM32F4XX_HAL_CONF_H
#define STM32F4XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Select HAL modules used by this firmware. */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED

/* Oscillator and board constants (BlackPill F411CE). */
#ifndef HSE_VALUE
#define HSE_VALUE    25000000U
#endif

#ifndef HSI_VALUE
#define HSI_VALUE    16000000U
#endif

#ifndef LSI_VALUE
#define LSI_VALUE    32000U
#endif

#ifndef LSE_VALUE
#define LSE_VALUE    32768U
#endif

#ifndef EXTERNAL_CLOCK_VALUE
#define EXTERNAL_CLOCK_VALUE 12288000U
#endif

#ifndef HSE_STARTUP_TIMEOUT
#define HSE_STARTUP_TIMEOUT 100U
#endif

#ifndef LSE_STARTUP_TIMEOUT
#define LSE_STARTUP_TIMEOUT 5000U
#endif

#define VDD_VALUE                    3300U
#define TICK_INT_PRIORITY            0x0FU
#if defined(USE_FREERTOS)
#define USE_RTOS                     1U
#else
#define USE_RTOS                     0U
#endif
#define PREFETCH_ENABLE              1U
#define INSTRUCTION_CACHE_ENABLE     1U
#define DATA_CACHE_ENABLE            1U

#define USE_FULL_ASSERT              0U

/* Export selected module headers. */
#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f4xx_hal_rcc.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f4xx_hal_gpio.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f4xx_hal_cortex.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32f4xx_hal_pwr.h"
#endif

#if (USE_FULL_ASSERT == 1U)
void assert_failed(uint8_t *file, uint32_t line);
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32F4XX_HAL_CONF_H */
