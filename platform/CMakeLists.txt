# platform — STM32F4 HAL/LL implementation of the abstract platform API
#
# USB is intentionally excluded from platform_stm32f4_hal so that the
# CDC-only and CDC+HID composite targets can each provide their own USB
# symbols without linker conflicts.  USB libraries are:
#   platform_usb_cdc_f4       — single CDC-class device (used by blinky)
#   platform_usb_composite_f4 — CDC + HID composite device (experimental)

add_library(platform_stm32f4_hal STATIC
    src/stm32f4/hal/platform_gpio.c
    src/stm32f4/hal/platform_clock.c
    src/stm32f4/hal/platform_i2c.c
    src/stm32f4/hal/platform_uart.c
    src/stm32f4/common/platform_init.c
)

target_include_directories(platform_stm32f4_hal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include   # exposes platform/gpio.h etc.
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32f4/hal
        ${CMSIS_INCLUDE_DIR}
        ${CMSIS_DEVICE_DIR}
        ${STM32CUBE_HAL_DIR}/Inc
        ${STM32CUBE_HAL_DIR}/Inc/Legacy
        ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include  # board.h for init
)

# STM32CubeF4 HAL sources (non-USB; USB HAL lives in the USB sub-libraries)
target_sources(platform_stm32f4_hal PRIVATE
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_cortex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_gpio.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_dma.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_i2c.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_uart.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_rcc.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_rcc_ex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pwr.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pwr_ex.c
)

# ─────────────────────────────────────────────────────────────────────────────
# Convenience path variable reused by both USB sub-libraries
# ─────────────────────────────────────────────────────────────────────────────
set(F4_USB_MW_DIR
    "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/ST/STM32_USB_Device_Library")

# ─────────────────────────────────────────────────────────────────────────────
# STM32F4 USB CDC single-class library  (blinky + console project)
# ─────────────────────────────────────────────────────────────────────────────
add_library(platform_usb_cdc_f4 STATIC
    src/stm32f4/hal/platform_usb_cdc.c
    src/stm32f4/hal/usbd_conf.c
    src/stm32f4/hal/usbd_desc.c
    src/stm32f4/hal/usbd_cdc_if.c
)

target_include_directories(platform_usb_cdc_f4
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32f4/hal
        ${CMSIS_INCLUDE_DIR}
        ${CMSIS_DEVICE_DIR}
        ${STM32CUBE_HAL_DIR}/Inc
        ${STM32CUBE_HAL_DIR}/Inc/Legacy
        ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include
        ${F4_USB_MW_DIR}/Core/Inc
        ${F4_USB_MW_DIR}/Class/CDC/Inc
)

target_sources(platform_usb_cdc_f4 PRIVATE
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pcd.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pcd_ex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_ll_usb.c
    ${F4_USB_MW_DIR}/Core/Src/usbd_core.c
    ${F4_USB_MW_DIR}/Core/Src/usbd_ctlreq.c
    ${F4_USB_MW_DIR}/Core/Src/usbd_ioreq.c
    ${F4_USB_MW_DIR}/Class/CDC/Src/usbd_cdc.c
)

target_link_libraries(platform_usb_cdc_f4 PUBLIC platform_stm32f4_hal)

# ─────────────────────────────────────────────────────────────────────────────
# STM32F4 USB CDC+HID composite library  (composite demo project)
# ─────────────────────────────────────────────────────────────────────────────
add_library(platform_usb_composite_f4 STATIC
    src/usb_composite/usbd_composite.c
    src/usb_composite/usbd_hid_kbd.c
    src/usb_composite/usbd_cdc_hid_desc.c
    src/usb_composite/usbd_cdc_if_composite.c
    src/stm32f4/composite/usbd_conf_composite.c
    src/stm32f4/composite/platform_usb_composite.c
)

target_include_directories(platform_usb_composite_f4
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_composite
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32f4/composite
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32f4/hal   # usbd_cdc_if.h
        ${CMSIS_INCLUDE_DIR}
        ${CMSIS_DEVICE_DIR}
        ${STM32CUBE_HAL_DIR}/Inc
        ${STM32CUBE_HAL_DIR}/Inc/Legacy
        ${CMAKE_SOURCE_DIR}/boards/blackpill_f411ce/include
        ${F4_USB_MW_DIR}/Core/Inc
        ${F4_USB_MW_DIR}/Class/CDC/Inc
)

target_sources(platform_usb_composite_f4 PRIVATE
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pcd.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_hal_pcd_ex.c
    ${STM32CUBE_HAL_DIR}/Src/stm32f4xx_ll_usb.c
    ${F4_USB_MW_DIR}/Core/Src/usbd_core.c
    ${F4_USB_MW_DIR}/Core/Src/usbd_ctlreq.c
    ${F4_USB_MW_DIR}/Core/Src/usbd_ioreq.c
    ${F4_USB_MW_DIR}/Class/CDC/Src/usbd_cdc.c
)

target_link_libraries(platform_usb_composite_f4 PUBLIC platform_stm32f4_hal)

# ─────────────────────────────────────────────────────────────────────────────
# STM32L0 platform library (optional, for STM32L0 Discovery board)
# ─────────────────────────────────────────────────────────────────────────────

if(STM32_USE_L0)
    set(STM32CUBE_L0_HAL_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubel0/Drivers/STM32L0xx_HAL_Driver")

    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/cmsis/Include")
        set(CMSIS_L0_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/cmsis/Include")
        set(CMSIS_L0_DEVICE_DIR  "${CMAKE_SOURCE_DIR}/third_party/cmsis/Device/ST/STM32L0xx/Include")
    else()
        set(CMSIS_L0_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/stm32cubel0/Drivers/CMSIS/Include")
        set(CMSIS_L0_DEVICE_DIR  "${CMAKE_SOURCE_DIR}/third_party/stm32cubel0/Drivers/CMSIS/Device/ST/STM32L0xx/Include")
    endif()

    add_library(platform_stm32l0_hal STATIC
        src/stm32l0/hal/platform_gpio.c
        src/stm32l0/hal/platform_clock.c
        src/stm32l0/hal/platform_i2c.c
        src/stm32l0/hal/platform_uart.c
        src/stm32l0/common/platform_init.c
    )

    target_include_directories(platform_stm32l0_hal
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32l0/hal
            ${CMSIS_L0_INCLUDE_DIR}
            ${CMSIS_L0_DEVICE_DIR}
            ${STM32CUBE_L0_HAL_DIR}/Inc
            ${STM32CUBE_L0_HAL_DIR}/Inc/Legacy
            ${CMAKE_SOURCE_DIR}/boards/stm32l0_discovery/include  # board.h
    )

    # STM32CubeL0 HAL source (only the files we actually use)
    target_sources(platform_stm32l0_hal PRIVATE
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_cortex.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_gpio.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_dma.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_i2c.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_uart.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_rcc.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_rcc_ex.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_pwr.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_pwr_ex.c
    )

    # ─── STM32L0 USB CDC+HID composite library ───────────────────────────────
    set(L0_USB_MW_DIR
        "${CMAKE_SOURCE_DIR}/third_party/stm32cubel0/Middlewares/ST/STM32_USB_Device_Library")

    add_library(platform_usb_composite_l0 STATIC
        src/usb_composite/usbd_composite.c
        src/usb_composite/usbd_hid_kbd.c
        src/usb_composite/usbd_cdc_hid_desc.c
        src/usb_composite/usbd_cdc_if_composite.c
        src/stm32l0/composite/usbd_conf_l0.c
        src/stm32l0/composite/platform_usb_composite.c
    )

    target_include_directories(platform_usb_composite_l0
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_composite
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32l0/composite
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32l0/hal   # usbd_cdc_if.h
            ${CMSIS_L0_INCLUDE_DIR}
            ${CMSIS_L0_DEVICE_DIR}
            ${STM32CUBE_L0_HAL_DIR}/Inc
            ${STM32CUBE_L0_HAL_DIR}/Inc/Legacy
            ${CMAKE_SOURCE_DIR}/boards/stm32l0_discovery/include
            ${L0_USB_MW_DIR}/Core/Inc
            ${L0_USB_MW_DIR}/Class/CDC/Inc
    )

    target_sources(platform_usb_composite_l0 PRIVATE
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_pcd.c
        ${STM32CUBE_L0_HAL_DIR}/Src/stm32l0xx_hal_pcd_ex.c
        ${L0_USB_MW_DIR}/Core/Src/usbd_core.c
        ${L0_USB_MW_DIR}/Core/Src/usbd_ctlreq.c
        ${L0_USB_MW_DIR}/Core/Src/usbd_ioreq.c
        ${L0_USB_MW_DIR}/Class/CDC/Src/usbd_cdc.c
    )

    target_link_libraries(platform_usb_composite_l0 PUBLIC platform_stm32l0_hal)
endif()
