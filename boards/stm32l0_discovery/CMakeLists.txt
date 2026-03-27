# board — pin mappings and Board_Init() for STM32L0 Discovery

add_library(board_stm32l0_discovery STATIC
    src/board.c
)

target_include_directories(board_stm32l0_discovery
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include   # exposes board.h
    PRIVATE
        ${CMSIS_INCLUDE_DIR}
        ${CMSIS_DEVICE_DIR}
        ${STM32CUBE_HAL_DIR}/Inc
)
