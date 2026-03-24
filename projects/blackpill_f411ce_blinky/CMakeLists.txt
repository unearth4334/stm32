# Project: blackpill_f411ce_blinky
# Produces: firmware.elf + firmware.bin + firmware.hex

set(TARGET firmware)

add_executable(${TARGET}
    main.c
    # Startup file — copy from third_party/cmsis or vendor startup/ folder
    # ${CMAKE_SOURCE_DIR}/startup/startup_stm32f411xe.s
)

target_include_directories(${TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include
    ${CMAKE_SOURCE_DIR}/platform/include
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        app
        services
        middleware
        drivers
        platform_stm32f4_hal
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
