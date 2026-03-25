# console — diagnostics logging and command interface

add_library(console STATIC
    src/console.c
)

target_include_directories(console
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include
        ${CMSIS_INCLUDE_DIR}
        ${CMSIS_DEVICE_DIR}
        ${STM32CUBE_HAL_DIR}/Inc
        ${STM32CUBE_HAL_DIR}/Inc/Legacy
)

target_link_libraries(console
    PUBLIC
        platform_stm32f4_hal
)