# platform — STM32F4 HAL/LL implementation of the abstract platform API

add_library(platform_stm32f4_hal STATIC
    src/stm32f4/hal/platform_gpio.c
    src/stm32f4/hal/platform_clock.c
    src/stm32f4/common/platform_init.c
)

target_include_directories(platform_stm32f4_hal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include   # exposes platform/gpio.h etc.
    PRIVATE
        ${CMSIS_INCLUDE_DIR}
        ${CMSIS_DEVICE_DIR}
        ${STM32CUBE_HAL_DIR}/Inc
        ${STM32CUBE_HAL_DIR}/Inc/Legacy
        ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include  # board.h for init
)

# STM32CubeF4 HAL source (only the files we actually use)
target_sources(platform_stm32f4_hal PRIVATE
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_cortex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_gpio.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_rcc.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_rcc_ex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pwr.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pwr_ex.c
)
