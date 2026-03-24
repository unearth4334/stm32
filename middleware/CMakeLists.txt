# middleware — reusable protocol/stack/runtime components

add_library(middleware STATIC
    rtos/rtos_kernel.c
)

target_include_directories(middleware
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include   # exposes middleware/rtos_kernel.h
)

# Optional FreeRTOS integration hook.
# When third_party/freertos is populated, set ENABLE_FREERTOS=ON.
target_compile_definitions(middleware
    PUBLIC
        $<$<BOOL:${ENABLE_FREERTOS}>:USE_FREERTOS>
)

if(ENABLE_FREERTOS)
    target_include_directories(middleware PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/freertos/FreeRTOS-Kernel/include
        ${CMAKE_SOURCE_DIR}/third_party/freertos/FreeRTOS-Kernel/portable/GCC/ARM_CM4F
    )
endif()

target_link_libraries(middleware PUBLIC platform_stm32f4_hal)
