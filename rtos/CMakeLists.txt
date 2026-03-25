# rtos — kernel integration targets

if(STM32_USE_FREERTOS)
    set(FREERTOS_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/Third_Party/FreeRTOS/Source")

    if(NOT EXISTS "${FREERTOS_DIR}/include/FreeRTOS.h")
        message(FATAL_ERROR "STM32_USE_FREERTOS is ON but FreeRTOS sources were not found at ${FREERTOS_DIR}. Populate third_party/stm32cubef4 first.")
    endif()

    add_library(freertos_kernel STATIC
        ${FREERTOS_DIR}/croutine.c
        ${FREERTOS_DIR}/event_groups.c
        ${FREERTOS_DIR}/list.c
        ${FREERTOS_DIR}/queue.c
        ${FREERTOS_DIR}/stream_buffer.c
        ${FREERTOS_DIR}/tasks.c
        ${FREERTOS_DIR}/timers.c
        ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
        ${FREERTOS_DIR}/portable/MemMang/heap_4.c
        freertos/freertos_hooks.c
    )

    target_include_directories(freertos_kernel
        PUBLIC
            ${FREERTOS_DIR}/include
            ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
            ${CMAKE_CURRENT_SOURCE_DIR}/freertos
        PRIVATE
            ${CMAKE_SOURCE_DIR}/console/include
            ${CMSIS_INCLUDE_DIR}
            ${CMSIS_DEVICE_DIR}
            ${STM32CUBE_HAL_DIR}/Inc
            ${STM32CUBE_HAL_DIR}/Inc/Legacy
            ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include
    )

    target_compile_definitions(freertos_kernel PUBLIC
        projCOVERAGE_TEST=0
    )
else()
    add_library(freertos_kernel INTERFACE)
endif()
