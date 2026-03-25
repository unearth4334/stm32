# app — top-level application logic

add_library(app STATIC
    app_blinky.c
    app_freertos.c
)

target_include_directories(app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}   # exposes app_blinky.h
)

target_link_libraries(app PUBLIC services osal console)
