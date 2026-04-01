# platform — STM32F4 HAL/LL implementation of the abstract platform API

add_library(platform_stm32f4_hal STATIC
    src/stm32f4/hal/platform_gpio.c
    src/stm32f4/hal/platform_clock.c
    src/stm32f4/hal/platform_i2c.c
    src/stm32f4/hal/platform_spi.c
    src/stm32f4/hal/platform_uart.c
    src/stm32f4/hal/platform_usb_cdc.c
    src/stm32f4/hal/usbd_conf.c
    src/stm32f4/hal/usbd_desc.c
    src/stm32f4/hal/usbd_cdc_if.c
    src/stm32f4/common/platform_init.c
)

target_include_directories(platform_stm32f4_hal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include   # exposes platform/gpio.h etc.
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32f4/hal
        ${CMSIS_INCLUDE_DIR}
        ${CMSIS_DEVICE_DIR}
        ${STM32CUBE_HAL_DIR}/Inc
        ${STM32CUBE_HAL_DIR}/Inc/Legacy
        ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include  # board.h for init
        ${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
        ${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
)

# STM32CubeF4 HAL source (only the files we actually use)
target_sources(platform_stm32f4_hal PRIVATE
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_cortex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_gpio.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_dma.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_i2c.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_spi.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_uart.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pcd.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pcd_ex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_ll_usb.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_rcc.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_rcc_ex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pwr.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pwr_ex.c
    ${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
    ${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
    ${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
    ${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
)
