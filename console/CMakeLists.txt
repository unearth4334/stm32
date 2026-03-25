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

if(STM32_USE_FREERTOS)
    set(FREERTOS_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/Third_Party/FreeRTOS/Source")
    target_include_directories(console PRIVATE
        ${FREERTOS_DIR}/include
        ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
        ${CMAKE_SOURCE_DIR}/rtos/freertos
    )
    target_link_libraries(console PUBLIC freertos_kernel)
endif()