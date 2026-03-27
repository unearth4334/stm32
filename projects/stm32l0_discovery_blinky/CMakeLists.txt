# Project: stm32l0_discovery_blinky
# Produces: firmware.elf + firmware.bin + firmware.hex

set(TARGET firmware)

set(PROJECT_SOURCES
    main.c
    stm32l0xx_it.c
)

set(STARTUP_FILE "${CMAKE_SOURCE_DIR}/startup/startup_stm32l052xx.s")
set(SYSTEM_FILE  "${CMAKE_SOURCE_DIR}/startup/system_stm32l0xx.c")

if(EXISTS "${STARTUP_FILE}")
    list(APPEND PROJECT_SOURCES "${STARTUP_FILE}")
endif()

if(EXISTS "${SYSTEM_FILE}")
    list(APPEND PROJECT_SOURCES "${SYSTEM_FILE}")
endif()

add_executable(${TARGET} ${PROJECT_SOURCES})

target_include_directories(${TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/boards/stm32l0_discovery/include
    ${CMAKE_SOURCE_DIR}/platform/include
    ${CMAKE_SOURCE_DIR}/app
    ${CMSIS_L0_INCLUDE_DIR}
    ${CMSIS_L0_DEVICE_DIR}
    ${STM32CUBE_L0_HAL_DIR}/Inc
    ${STM32CUBE_L0_HAL_DIR}/Inc/Legacy
)

target_link_libraries(${TARGET}
    PRIVATE
        app
        services
        drivers
        platform_stm32l0_hal
        board_stm32l0_discovery
)

# Linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/stm32l052.ld")
target_link_options(${TARGET} PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=${TARGET}.map,--cref
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)
