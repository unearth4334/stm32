# Project: blackpill_f411ce_composite
# Produces: firmware-composite.elf + .bin + .hex
#
# Links platform_usb_composite_f4 which already pulls in platform_stm32f4_hal,
# so GPIO / UART / clock HAL sources are available without a separate dep.

set(TARGET firmware-composite)

set(PROJECT_SOURCES
    main.c
    stm32f4xx_it.c
)

set(STARTUP_FILE "${CMAKE_SOURCE_DIR}/startup/startup_stm32f411xe.s")
set(SYSTEM_FILE  "${CMAKE_SOURCE_DIR}/startup/system_stm32f4xx.c")

if(EXISTS "${STARTUP_FILE}")
    list(APPEND PROJECT_SOURCES "${STARTUP_FILE}")
endif()

if(EXISTS "${SYSTEM_FILE}")
    list(APPEND PROJECT_SOURCES "${SYSTEM_FILE}")
endif()

add_executable(${TARGET} ${PROJECT_SOURCES})

target_include_directories(${TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include
    ${CMAKE_SOURCE_DIR}/platform/include
    ${CMSIS_INCLUDE_DIR}
    ${CMSIS_DEVICE_DIR}
    ${STM32CUBE_HAL_DIR}/Inc
    ${STM32CUBE_HAL_DIR}/Inc/Legacy
)

target_link_libraries(${TARGET}
    PRIVATE
        platform_usb_composite_f4
        board_blackpill_f411ce
)

# Linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/stm32f411.ld")
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
