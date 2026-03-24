# osal — operating system abstraction layer

add_library(osal STATIC
    src/baremetal/osal_delay_baremetal.c
    src/baremetal/osal_task_baremetal.c
)

target_include_directories(osal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(osal PUBLIC platform_stm32f4_hal)
