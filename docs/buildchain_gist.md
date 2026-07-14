# Application Note: Implementing a GitHub Actions Buildchain for STM32F411 BlackPill

## 1. Purpose

This application note documents a practical, repeatable GitHub Actions buildchain
for an STM32F411CE BlackPill firmware project using CMake and the GNU Arm Embedded
Toolchain.

The goal is to produce deterministic firmware artifacts in CI:

• firmware.elf
• firmware.bin
• firmware.hex
• firmware.map

The buildchain validates both bare-metal and FreeRTOS-enabled build paths.

## 2. Scope and Assumptions

• Target MCU: STM32F411xE (BlackPill)
• Build system: CMake + Ninja
• Toolchain: arm-none-eabi-gcc
• CI platform: GitHub-hosted Ubuntu runner
• Vendor HAL/CMSIS source: STM32CubeF4
• RTOS support: FreeRTOS (optional, compile-time controlled)
• Optional middleware: USB device stack (from Cube)

The workflow is defined in:

• `.github/workflows/blinky-build.yml`

This implementation has been validated on the `main` branch with FreeRTOS and USB
middleware dependencies properly integrated.

## 3. Workflow Architecture

Pipeline stages:

1. Checkout repository
2. Install build dependencies (toolchain + build tools)
3. Populate STM32CubeF4 vendor sources
4. Initialize required vendor submodules
5. Populate startup and system source files
6. Configure CMake (cross-compile, with FreeRTOS enabled)
7. Build firmware target
8. Upload artifacts (with always-run and fallback semantics)

## 4. Key Implementation Details

### 4.1 Toolchain and Build Packages

On `ubuntu-latest`, install:

• cmake
• ninja-build
• gcc-arm-none-eabi
• binutils-arm-none-eabi
• libnewlib-arm-none-eabi

Reason: bare-metal linker specs and libc/newlib dependencies are required for
reliable configure/build behavior. The newlib package specifically provides
`--specs=nano.specs` and linking support for bare-metal targets.

### 4.2 CMake Cross-Compile Hardening

In `cmake/arm-none-eabi.cmake`:

• Set `CMAKE_TRY_COMPILE_TARGET_TYPE` to `STATIC_LIBRARY`.

Reason: avoid configure-time link tests that are often invalid for bare-metal
targets and can fail before real target compilation.

The toolchain also sets:

```cmake
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs")
```

This ensures standard Cortex-M4F ABI and no OS syscall stubs in the final image.

### 4.3 Vendor Source Population

The workflow uses a fresh clone + explicit submodule init strategy:

```bash
rm -rf third_party/stm32cubef4
git clone --depth 1 https://github.com/STMicroelectronics/stm32cubef4 third_party/stm32cubef4
git -C third_party/stm32cubef4 submodule update --init --depth 1 --recommend-shallow \
  Drivers/CMSIS/Device/ST/STM32F4xx \
  Drivers/STM32F4xx_HAL_Driver \
  Middlewares/Third_Party/FreeRTOS \
  Middlewares/ST/STM32_USB_Device_Library
```

Reason: 

- Shallow clone reduces CI artifact size and download time.
- Explicit submodule list ensures reproducible availability:
  - HAL/CMSIS headers and implementations (always required)
  - FreeRTOS kernel sources (required when FreeRTOS-enabled)
  - USB device middleware (required by console diagnostics module)

### 4.4 Startup Source Files

After populating Cube, validate and copy startup/system files:

```bash
STARTUP_SRC=third_party/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s
SYSTEM_SRC=third_party/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c

test -f "$STARTUP_SRC" || { echo "Missing startup source: $STARTUP_SRC"; ... ; exit 1; }
test -f "$SYSTEM_SRC" || { echo "Missing system source: $SYSTEM_SRC"; ... ; exit 1; }

mkdir -p startup
cp "$STARTUP_SRC" startup/
cp "$SYSTEM_SRC" startup/
```

Reason:

- Explicit validation catches Cube layout changes early.
- Diagnostics help when Cube fetch fails.
- Local startup/ folder is referenced in project CMakeLists.txt for consistency.

### 4.5 HAL Configuration Header

Provide a board-specific HAL config header:

• `boards/blackpill_f411ce/include/stm32f4xx_hal_conf.h`

Must include required module enables and timeout/assert macros expected by selected
HAL sources. Critical settings on STM32F4:

```c
#define USE_HAL_GPIO_REGISTER_CALLBACKS 1
#define USE_HAL_UART_REGISTER_CALLBACKS 1
#define USE_RTOS 0U   /* Do NOT set to 1 even with FreeRTOS enabled */
```

The `USE_RTOS` flag provides no scheduler integration value for STM32F4 HAL; RTOS
interaction happens through the OSAL layer and explicit hook functions instead.

### 4.6 Include Path Consistency

Ensure include usage and CMake include directories are aligned:

• Avoid fragile prefixed self-includes that conflict with current include roots.
• Add HAL/CMSIS includes to targets that compile sources including `board.h` or
  `stm32f4xx_hal.h`.

Targets adjusted in current build:

• `platform_stm32f4_hal` (platform layer)
• `board_blackpill_f411ce` (board config)
• `console` (diagnostics; requires HAL for UART/USB APIs)
• `freertos_kernel` (when FreeRTOS enabled)

CMake properly propagates include paths via `target_include_directories` with
`PUBLIC` / `PRIVATE` scoping to avoid over-including.

### 4.7 FreeRTOS-Enabled Configuration

The CI pipeline configures with:

```bash
cmake -S . -B build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DSTM32_USE_FREERTOS=ON
```

This enables:

- FreeRTOS kernel target (`rtos/CMakeLists.txt`)
- OSAL FreeRTOS backend (`osal/src/freertos/`)
- Console diagnostics with stream buffer transport
- Runtime task wiring in `projects/blackpill_f411ce_blinky/main.c`

The `USE_FREERTOS` compile definition is automatically added when
`STM32_USE_FREERTOS=ON`.

### 4.8 SysTick and HAL_Delay Runtime Behavior

Add SysTick IRQ handler source for firmware runtime:

• `projects/blackpill_f411ce_blinky/stm32f4xx_it.c`

Implementation:

```c
void SysTick_Handler(void)
{
#if defined(USE_FREERTOS)
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
#endif
    HAL_IncTick();
}
```

Reason: `HAL_Delay` depends on `uwTick` incremented by SysTick. Without SysTick
handling, blinky loops can appear to run once and then stall. When FreeRTOS is
enabled, the handler also ensures the kernel tick is serviced once the scheduler
starts.

### 4.9 Active-Low LED Initialization

For BlackPill PC13 user LED (active-low), initialize OFF as logic high:

• `boards/blackpill_f411ce/src/board.c`

Reason: incorrect initial polarity can make runtime behavior appear inverted or
apparently stuck.

### 4.10 Interrupt Handlers for Transport and Fault Recording

The project includes interrupt handlers for console transport and diagnostics:

```c
void USART1_IRQHandler(void)
{
    platform_uart_irq_handler(platform_uart_debug_handle());
}

void OTG_FS_IRQHandler(void)
{
    platform_usb_cdc_irq_handler();
}

void HardFault_Handler(void)
{
    console_record_fault("hardfault", 0x48465254U, SCB->CFSR, SCB->HFSR);
    console_log_panic("fault", "hardfault cfsr=0x%08lX hfsr=0x%08lX", ...);
}
```

These ensure diagnostics are available immediately on fault, and console I/O is
responsive in both bare-metal and FreeRTOS modes.

## 5. Artifact Publishing

Artifacts are uploaded with `actions/upload-artifact@v4`:

```yaml
- name: Upload build artifacts
  if: always()
  uses: actions/upload-artifact@v4
  with:
    name: firmware-artifacts
    path: |
      build/projects/blackpill_f411ce_blinky/firmware.elf
      build/projects/blackpill_f411ce_blinky/firmware.bin
      build/projects/blackpill_f411ce_blinky/firmware.hex
      build/projects/blackpill_f411ce_blinky/firmware.map
    if-no-files-found: warn
```

Guarded by `if: always()` to ensure artifacts are uploaded even if build fails
(useful for debugging). The `if-no-files-found: warn` provides visibility into
missing artifacts without failing the workflow.

## 6. Flashing Procedure (DFU)

A typical DFU flash command with STM32CubeProgrammer CLI:

1. Detect device:

```
STM32_Programmer_CLI.exe -l usb
```

2. Program and verify:

```
STM32_Programmer_CLI.exe -c port=USB1 -w firmware.bin 0x08000000 -v
```

After programming:

• Set BOOT0 low
• Reset board to boot from user flash

## 7. Troubleshooting Checklist

If CI fails, check in this order:

1. Vendor submodules initialized (check `third_party/stm32cubef4/Middlewares`)
2. Startup/system files copied and present in `startup/`
3. HAL config header exists at `boards/blackpill_f411ce/include/stm32f4xx_hal_conf.h`
4. `USE_RTOS` is `0U` in HAL config (not `1`)
5. Target include directories contain HAL/CMSIS where needed
6. Cross-toolchain package set includes newlib
7. `CMAKE_TRY_COMPILE_TARGET_TYPE` is `STATIC_LIBRARY` in toolchain file
8. Runtime SysTick handler is linked for HAL_Delay users
9. FreeRTOS hooks wired (malloc failure, stack overflow → console diagnostics)
10. Console diagnostics module initialized before scheduler start

## 8. Recommended Maintenance Practices

1. Pin action versions (e.g., `actions/checkout@v4`, `actions/upload-artifact@v4`)
   and keep changelog notes in commits.
2. Keep CI diagnostics explicit for missing files and paths.
3. Prefer minimal target-local include paths over broad global includes.
4. Treat CI logs as the source of truth for iterative stabilization.
5. Keep artifact names stable for automation scripts and flashing workflows.
6. When updating STM32CubeF4 version, validate FreeRTOS submodule availability
   (paths may shift between Cube releases).
7. Run FreeRTOS-enabled CI path regularly to avoid drift in optional feature
   support.

## 9. Outcome

The GitHub Actions buildchain is operational and produces flashable firmware
artifacts for STM32F411 BlackPill with:

• Deterministic CI behavior across bare-metal and FreeRTOS builds
• Reproducible vendor dependency management
• Clear diagnostic output for troubleshooting
• Validated diagnostics/fault telemetry path (console module)
• Integration with both UART and USB device transport