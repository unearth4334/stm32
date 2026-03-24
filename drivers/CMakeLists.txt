# drivers — hardware-agnostic driver implementations

add_library(drivers STATIC
    led/led_driver.c
)

target_include_directories(drivers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}   # exposes drivers/led/led_driver.h
    PRIVATE
        ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include  # board.h for pin defs
)

target_link_libraries(drivers PUBLIC platform_stm32f4_hal)
