# Project: stm32l0_composite
# Produces: firmware-composite.elf + .bin + .hex
#
# Links platform_usb_composite_l0 which pulls in platform_stm32l0_hal, so
# GPIO / UART / clock HAL sources are available without a separate dep.

set(TARGET firmware-composite)

set(PROJECT_SOURCES
    main.c
    stm32l0xx_it.c
)

# Startup and system files are populated by the CI workflow (or manually)
if(DEFINED CMSIS_L0_DEVICE_DIR)
    set(STARTUP_FILE "${CMSIS_L0_DEVICE_DIR}/../Source/Templates/gcc/startup_stm32l052xx.s")
    set(SYSTEM_FILE  "${CMSIS_L0_DEVICE_DIR}/../Source/Templates/system_stm32l0xx.c")
else()
    set(STARTUP_FILE "${CMAKE_SOURCE_DIR}/startup/startup_stm32l052xx.s")
    set(SYSTEM_FILE  "${CMAKE_SOURCE_DIR}/startup/system_stm32l0xx.c")
endif()

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
    ${CMSIS_L0_INCLUDE_DIR}
    ${CMSIS_L0_DEVICE_DIR}
    ${STM32CUBE_L0_HAL_DIR}/Inc
    ${STM32CUBE_L0_HAL_DIR}/Inc/Legacy
)

target_link_libraries(${TARGET}
    PRIVATE
        platform_usb_composite_l0
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
set_target_properties(${TARGET} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Post-build: generate .bin and .hex
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET}> ${TARGET}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${TARGET}> ${TARGET}.hex
    COMMAND ${CMAKE_SIZE}              $<TARGET_FILE:${TARGET}>
    COMMENT "Generating ${TARGET}.bin and ${TARGET}.hex"
)
