/**
 * @file stm32l0xx_hal_conf.h
 * @brief STM32L0 HAL Configuration Header
 *
 * Selects which HAL drivers to compile into the build.
 */

#ifndef STM32L0XX_HAL_CONFIG_H
#define STM32L0XX_HAL_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* #define HAL_MODULE_ENABLED */

#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED

#define HSE_VALUE    8000000U  /*!< Value of the external oscillator in Hz */
#define MSI_VALUE    2097000U  /*!< Value of the Multiple Speed Internal oscillator in Hz */
#define EXTERNAL_CLOCK_VALUE  2097000U  /*!< External clock value in Hz if used as system clock source */

#define USE_HAL_RCC_REGISTER_CALLBACKS 0U
#define USE_HAL_CORTEX_REGISTER_CALLBACKS 0U
#define USE_HAL_FLASH_REGISTER_CALLBACKS 0U
#define USE_HAL_GPIO_REGISTER_CALLBACKS 0U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_USART_REGISTER_CALLBACKS 0U
#define USE_HAL_I2C_REGISTER_CALLBACKS 0U
#define USE_HAL_PWR_REGISTER_CALLBACKS 0U
#define USE_HAL_RTC_REGISTER_CALLBACKS 0U
#define USE_HAL_SPI_REGISTER_CALLBACKS 0U
#define USE_HAL_TIM_REGISTER_CALLBACKS 0U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_USART_REGISTER_CALLBACKS 0U
#define USE_HAL_LPUART_REGISTER_CALLBACKS 0U

#define PREFETCH_ENABLE        0U
#define INSTRUCTION_CACHE_ENABLE 0U
#define DATA_CACHE_ENABLE      0U

/* Includes ------------------------------------------------------------------*/
/**
  * @brief Include module's header file
  */

#ifdef HAL_RCC_MODULE_ENABLED
 #include "stm32l0xx_hal_rcc.h"
#endif /* HAL_RCC_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
 #include "stm32l0xx_hal_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */

#ifdef HAL_DMA_MODULE_ENABLED
 #include "stm32l0xx_hal_dma.h"
#endif /* HAL_DMA_MODULE_ENABLED */

#ifdef HAL_UART_MODULE_ENABLED
 #include "stm32l0xx_hal_uart.h"
#endif /* HAL_UART_MODULE_ENABLED */

#ifdef HAL_USART_MODULE_ENABLED
 #include "stm32l0xx_hal_usart.h"
#endif /* HAL_USART_MODULE_ENABLED */

#ifdef HAL_I2C_MODULE_ENABLED
 #include "stm32l0xx_hal_i2c.h"
#endif /* HAL_I2C_MODULE_ENABLED */

#ifdef HAL_CORTEX_MODULE_ENABLED
 #include "stm32l0xx_hal_cortex.h"
#endif /* HAL_CORTEX_MODULE_ENABLED */

#ifdef HAL_PWR_MODULE_ENABLED
 #include "stm32l0xx_hal_pwr.h"
#endif /* HAL_PWR_MODULE_ENABLED */

#ifdef HAL_FLASH_MODULE_ENABLED
 #include "stm32l0xx_hal_flash.h"
#endif /* HAL_FLASH_MODULE_ENABLED */

/* Exported types and variables -----------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* STM32L0XX_HAL_CONFIG_H */
