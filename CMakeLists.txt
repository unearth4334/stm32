cmake_minimum_required(VERSION 3.22)

# ── Toolchain must be set before project() ─────────────────────────────────
# Pass via CLI: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake ..
# Or set here for convenience when always cross-compiling:
# set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake")

project(stm32_firmware C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Board selection ─────────────────────────────────────────────────────────
option(STM32_USE_L0 "Build for STM32L0 Discovery board instead of STM32F4" OFF)
option(STM32_USE_FREERTOS "Enable FreeRTOS-oriented startup path" OFF)
option(USE_USB_COMPOSITE "Build the experimental CDC+HID composite USB demo" OFF)

# ── Common compile definitions ──────────────────────────────────────────────
add_compile_definitions(USE_HAL_DRIVER)

if(STM32_USE_L0)
    add_compile_definitions(STM32L052xx)
else()
    add_compile_definitions(STM32F411xE)
endif()

if(STM32_USE_FREERTOS)
    add_compile_definitions(USE_FREERTOS=1)
endif()

# ── Common include paths shared across all libraries ──────────────────────
# Prefer a dedicated third_party/cmsis checkout when available.
# Fallback to STM32Cube-provided CMSIS layout for CI and simple setups.

if(STM32_USE_L0)
    set(STM32CUBE_HAL_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubel0/Drivers/STM32L0xx_HAL_Driver")
    
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/cmsis/Include")
        set(CMSIS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/cmsis/Include")
        set(CMSIS_DEVICE_DIR  "${CMAKE_SOURCE_DIR}/third_party/cmsis/Device/ST/STM32L0xx/Include")
    else()
        set(CMSIS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubel0/Drivers/CMSIS/Include")
        set(CMSIS_DEVICE_DIR  "${CMAKE_SOURCE_DIR}/third_party/stm32cubel0/Drivers/CMSIS/Device/ST/STM32L0xx/Include")
    endif()
else()
    set(STM32CUBE_HAL_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Drivers/STM32F4xx_HAL_Driver")

    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/cmsis/Include")
        set(CMSIS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/cmsis/Include")
        set(CMSIS_DEVICE_DIR  "${CMAKE_SOURCE_DIR}/third_party/cmsis/Device/ST/STM32F4xx/Include")
    else()
        set(CMSIS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Drivers/CMSIS/Include")
        set(CMSIS_DEVICE_DIR  "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Include")
    endif()
endif()

# ── Sub-libraries ───────────────────────────────────────────────────────────
add_subdirectory(platform)
add_subdirectory(rtos)
add_subdirectory(osal)
add_subdirectory(console)
add_subdirectory(drivers)
add_subdirectory(services)
add_subdirectory(app)

# ── Projects (each project defines its own executable) ─────────────────────
if(STM32_USE_L0)
    add_subdirectory(boards/stm32l0_discovery)
    add_subdirectory(projects/stm32l0_discovery_blinky)
    if(USE_USB_COMPOSITE)
        add_subdirectory(projects/stm32l0_composite)
    endif()
else()
    add_subdirectory(boards/blackpill_f411ce)
    add_subdirectory(projects/blackpill_f411ce_blinky)
    if(USE_USB_COMPOSITE)
        add_subdirectory(projects/blackpill_f411ce_composite)
    endif()
endif()

# ── Tests (host-side only, skip when cross-compiling) ──────────────────────
if(NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()
