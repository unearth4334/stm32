# osal — operating system abstraction layer

if(STM32_USE_FREERTOS)
    add_library(osal STATIC
        src/freertos/osal_delay_freertos.c
        src/freertos/osal_task_freertos.c
    )
else()
    add_library(osal STATIC
        src/baremetal/osal_delay_baremetal.c
        src/baremetal/osal_task_baremetal.c
    )
endif()

target_include_directories(osal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(STM32_USE_FREERTOS)
    target_link_libraries(osal PUBLIC platform_stm32f4_hal freertos_kernel)
else()
    target_link_libraries(osal PUBLIC platform_stm32f4_hal)
endif()
