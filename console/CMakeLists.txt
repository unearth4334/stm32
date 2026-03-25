# console — diagnostics logging and command interface

add_library(console STATIC
    src/console.c
)

target_include_directories(console
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(console
    PUBLIC
        platform_stm32f4_hal
)