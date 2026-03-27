# services — business-logic wrappers over drivers

add_library(services STATIC
    led_service.c
)

target_include_directories(services
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}   # exposes services/led_service.h
)

target_link_libraries(services PUBLIC drivers)
