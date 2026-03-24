cmake_minimum_required(VERSION 3.22)

# ── Toolchain must be set before project() ─────────────────────────────────
# Pass via CLI: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake ..
# Or set here for convenience when always cross-compiling:
# set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake")

project(stm32f4_firmware C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Common compile definitions ──────────────────────────────────────────────
add_compile_definitions(
    USE_HAL_DRIVER
    STM32F411xE
)

# ── Common include paths shared across all libraries ──────────────────────
# Prefer a dedicated third_party/cmsis checkout when available.
# Fallback to STM32CubeF4-provided CMSIS layout for CI and simple setups.
set(STM32CUBE_HAL_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Drivers/STM32F4xx_HAL_Driver")

if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/cmsis/Include")
    set(CMSIS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/cmsis/Include")
    set(CMSIS_DEVICE_DIR  "${CMAKE_SOURCE_DIR}/third_party/cmsis/Device/ST/STM32F4xx/Include")
else()
    set(CMSIS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Drivers/CMSIS/Include")
    set(CMSIS_DEVICE_DIR  "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Include")
endif()

# ── Sub-libraries ───────────────────────────────────────────────────────────
add_subdirectory(platform)
add_subdirectory(drivers)
add_subdirectory(services)
add_subdirectory(app)

# ── Projects (each project defines its own executable) ─────────────────────
add_subdirectory(boards/blackpill_f411ce)
add_subdirectory(projects/blackpill_f411ce_blinky)

# ── Tests (host-side only, skip when cross-compiling) ──────────────────────
if(NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()
