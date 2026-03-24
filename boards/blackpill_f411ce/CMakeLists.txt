# board — pin mappings and board_init() for WeAct STM32F411CE BlackPill

add_library(board_blackpill_f411ce STATIC
    src/board.c
)

target_include_directories(board_blackpill_f411ce
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include   # exposes board.h
    PRIVATE
        ${CMSIS_INCLUDE_DIR}
        ${CMSIS_DEVICE_DIR}
        ${STM32CUBE_HAL_DIR}/Inc
)
