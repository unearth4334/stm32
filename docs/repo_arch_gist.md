# Application Note

## STM32F4-Specific Repo Architecture for Robust, Portable Firmware

---

## 1. Scope

This document defines a production-grade repository architecture for STM32F4-based embedded C
projects, optimized for:

- Multi-board support (Nucleo, Discovery, custom hardware)
- Portability across STM32F4 variants (F401, F407, F411, F429, F446)
- Clean separation between application, hardware abstraction, and board configuration
- Long-term maintainability and scalability
- Optional FreeRTOS integration without RTOS sprawl

---

## 2. Core Design Principles

- Treat STM32CubeF4 as vendor code, not architecture
- Make STM32F4 family a first-class layer
- Make board configuration a first-class layer
- Keep application and drivers hardware-agnostic
- Use HAL + LL selectively, behind abstraction
- Use an OSAL layer when introducing an RTOS

---

## 3. Layered Architecture

### Bare-metal

```
Application
    G��
Services
    G��
Drivers
    G��
Platform (STM32F4 abstraction)
    G��
Board (hardware config)
    G��
STM32CubeF4 + CMSIS
```

### With FreeRTOS

```
Application / Tasks
    G��
Services
    G��
Drivers
    G��
OSAL (OS Abstraction Layer)    G�� added
    G��
Platform (STM32F4 abstraction)
    G��
Board (hardware config)
    G��
STM32CubeF4 + CMSIS + FreeRTOS
```

The OSAL layer ensures application code, services, and drivers never include FreeRTOS headers
directly. This keeps them portable, testable, and insulated from RTOS policy changes.

---

## 4. Recommended Repo Structure

```
repo/
G��G�� app/
G��G�� services/
G��G�� drivers/
G��G�� osal/                          # OS abstraction layer (add when introducing RTOS)
G��  G��G�� include/osal/
G��  G��  G��G�� task.h
G��  G��  G��G�� delay.h
G��  G��  G��G�� mutex.h
G��  G��  G��G�� queue.h
G��  G��  G��G�� event.h
G��  G��  G��G�� timer.h
G��  G��  G��G�� critical.h
G��  G��G�� src/
G��     G��G�� freertos/
G��     G��  G��G�� osal_task_freertos.c
G��     G��  G��G�� osal_delay_freertos.c
G��     G��  G��G�� osal_mutex_freertos.c
G��     G��  G��G�� osal_queue_freertos.c
G��     G��  G��G�� osal_event_freertos.c
G��     G��G�� baremetal/
G��        G��G�� osal_task_baremetal.c
G��        G��G�� osal_delay_baremetal.c
G��G�� rtos/                          # RTOS policy and kernel config
G��  G��G�� freertos/
G��     G��G�� FreeRTOSConfig.h
G��     G��G�� freertos_hooks.c
G��     G��G�� (heap/startup policies)
G��G�� platform/
G��  G��G�� include/platform/
G��  G��G�� src/stm32f4/
G��     G��G�� hal/
G��     G��G�� ll/
G��     G��G�� common/
G��     G��G�� startup/
G��G�� boards/
G��  G��G�� nucleo_f446re/
G��  G��G�� disco_f429zi/
G��  G��G�� custom_f407_rev_a/
G��  G��G�� custom_f411_rev_b/
G��G�� third_party/
G��  G��G�� cmsis/
G��  G��G�� stm32cubef4/
G��     G��G�� Middlewares/Third_Party/FreeRTOS/  # kernel source
G��G�� linker/
G��  G��G�� stm32f401.ld
G��  G��G�� stm32f407.ld
G��  G��G�� stm32f411.ld
G��  G��G�� stm32f429.ld
G��  G��G�� stm32f446.ld
G��G�� startup/
G��G�� tests/
G��G�� projects/
```

---

## 5. STM32F4 Platform Layer

This is where all STM32F4-specific implementation lives.

### Responsibilities

- HAL / LL wrappers
- IRQ handling
- DMA configuration helpers
- Clock helpers (RCC)
- Low-level init (cache, FPU, etc.)

### Example

```c
// platform/include/platform/i2c.h
int platform_i2c_write(...);

// platform/src/stm32f4/hal/platform_i2c.c
HAL_I2C_Master_Transmit(...);
```

---

## 6. Board Layer (Critical for Portability)

Each board defines:

- Pin mappings
- Peripheral instances
- Clock tree
- External components
- Hardware quirks

### Example

```c
#define BOARD_LED_PORT GPIOB
#define BOARD_LED_PIN  GPIO_PIN_0

void board_init(void)
{
    MX_GPIO_Init();
    MX_I2C1_Init();
}
```

---

## 7. HAL vs LL Strategy

| Use Case | Recommendation |
|---|---|
| General peripherals | HAL |
| Performance-critical | LL |
| Tight timing / ISR | LL or registers |
| Complex stacks (USB, ETH) | HAL |

---

## 8. Build System (CMake Recommended)

### Bare-metal targets

```cmake
add_library(platform_stm32f4_hal ...)
add_library(board_custom_f407 ...)
add_library(app_core ...)

target_link_libraries(firmware
    platform_stm32f4_hal
    board_custom_f407
    app_core
)
```

### With FreeRTOS (extended targets)

```cmake
add_library(freertos_kernel ...)
add_library(osal ...)
add_library(platform_stm32f4_hal ...)
add_library(board_custom_f407 ...)
add_library(app_core ...)

target_link_libraries(firmware
    PRIVATE
    freertos_kernel
    osal
    platform_stm32f4_hal
    board_custom_f407
    app_core
)
```

---

## 9. Configuration Strategy

| Type | Location |
|---|---|
| Compile-time | app_config.h |
| Board config | boards/\<board\>/include |
| Runtime | services/config_store |
| RTOS policy | rtos/freertos/FreeRTOSConfig.h |

---

## 10. Linker + Startup Strategy

- One linker script per MCU
- Separate startup files per family if needed
- Keep bootloader/app separation clean

### Memory sizing with FreeRTOS

Increase linker reserves when enabling an RTOS:

| Region | Bare-metal | With FreeRTOS |
|---|---|---|
| Heap   | 1 KB       | 8 KB minimum  |
| Stack  | 2 KB       | 4 KB minimum  |

These are starting values for a single-task application. Add roughly 512G��1024 bytes per
additional task depending on maximum stack depth.

---

## 11. Testing Strategy

### Host-side

- App logic
- Drivers (mock platform)
- OSAL mock backend (replaces FreeRTOS in unit tests)

### Target

- Platform implementation
- Timing / IRQ / DMA

---

## 12. Dependency Rules

### Bare-metal

```
app G�� services G�� drivers G�� platform G�� board G�� vendor
```

### With FreeRTOS

```
app/tasks G�� services G�� drivers G�� osal G�� platform G�� board G�� vendor + FreeRTOS
```

Never reverse dependencies. Only `osal/src/freertos/` and `rtos/freertos/` files should ever
include FreeRTOS kernel headers (`FreeRTOS.h`, `task.h`, `queue.h`, etc.). Enforcing this
prevents RTOS sprawl across the codebase.

---

## 13. Anti-Patterns

- Using CubeMX structure as final architecture
- Drivers including HAL directly
- App referencing GPIO pins
- Global HAL handles everywhere
- Massive `#ifdef` usage for boards
- Including `FreeRTOS.h` directly in app, services, or driver code
- Calling `xTaskCreate` outside the OSAL layer
- Setting `USE_RTOS 1` in `stm32f4xx_hal_conf.h` G�� hard compile error in STM32F4 HAL
- Including `stm32f4xx.h` in `FreeRTOSConfig.h` G�� causes cascade build failures
- Missing SVC or PendSV handlers when using FreeRTOS G�� tasks silently never run

---

## 14. Minimal Example

```
app/logger.c
drivers/tmp117.c
platform/stm32f4/i2c.c
boards/rev_a/board.c
```

---

## 15. Key Principle

STM32CubeF4 is an implementation detail G�� not your architecture.

---

## 16. FreeRTOS OSAL Interface Design

The OSAL layer exposes project-owned primitives that hide FreeRTOS kernel types. Application
and driver code depends only on these headers G�� never on `FreeRTOS.h` directly.

```c
// osal/include/osal/task.h
typedef void (*osal_task_fn_t)(void *arg);

typedef struct {
    const char  *name;
    uint16_t     stack_words;
    uint8_t      priority;
} osal_task_config_t;

int  osal_task_create(osal_task_fn_t fn, void *arg, const osal_task_config_t *cfg);
void osal_scheduler_start(void);

// osal/include/osal/delay.h
void osal_delay_ms(uint32_t ms);
```

Implement each header twice: once in `osal/src/freertos/` wrapping FreeRTOS calls, once in
`osal/src/baremetal/` as a compile-time fallback.

---

## 17. FreeRTOS CMake Backend Selection

Select the OSAL backend at configure time via a CMake option:

```cmake
# Root CMakeLists.txt
option(STM32_USE_FREERTOS "Enable FreeRTOS-oriented startup path" OFF)
if(STM32_USE_FREERTOS)
    add_compile_definitions(USE_FREERTOS=1)
endif()

add_subdirectory(rtos)
add_subdirectory(osal)
```

```cmake
# osal/CMakeLists.txt
add_library(osal STATIC)
target_include_directories(osal PUBLIC include)

if(STM32_USE_FREERTOS)
    target_sources(osal PRIVATE
        src/freertos/osal_delay_freertos.c
        src/freertos/osal_task_freertos.c
    )
    target_link_libraries(osal PUBLIC freertos_kernel)
else()
    target_sources(osal PRIVATE
        src/baremetal/osal_delay_baremetal.c
        src/baremetal/osal_task_baremetal.c
    )
endif()
```

```cmake
# rtos/CMakeLists.txt G�� FreeRTOS kernel static library
set(FREERTOS_DIR
    "${CMAKE_SOURCE_DIR}/third_party/stm32cubef4/Middlewares/Third_Party/FreeRTOS/Source")

add_library(freertos_kernel STATIC
    ${FREERTOS_DIR}/croutine.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
    freertos/freertos_hooks.c
)

target_include_directories(freertos_kernel
    PUBLIC
        ${FREERTOS_DIR}/include
        ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
        ${CMAKE_CURRENT_SOURCE_DIR}/freertos
    PRIVATE
        ${CMAKE_SOURCE_DIR}/boards/<your_board>/include   # needed for stm32f4xx_hal_conf.h
)
```

---

## 18. FreeRTOS Application Layer Changes

With FreeRTOS, `main()` becomes a thin bootstrap. Application logic moves into tasks:

```c
// main.c
#if defined(USE_FREERTOS)
    app_rtos_init();
    osal_scheduler_start();
#else
    app_blinky_init();
    while (1) { app_blinky_run(); }
#endif
```

```c
// app/app_freertos.c
static void blinky_task(void *arg) {
    app_blinky_init();
    while (1) { app_blinky_run(); }
}

void app_rtos_init(void) {
    const osal_task_config_t cfg = {
        .name        = "blinky",
        .stack_words = 256,
        .priority    = 1
    };
    osal_task_create(blinky_task, NULL, &cfg);
}
```

As the application grows, add an `app/tasks/` subtree:

```
app/
G��G�� include/app/
G��G�� src/
G��  G��G�� app_init.c
G��  G��G�� app_state.c
G��G�� tasks/
   G��G�� comms_task.c
   G��G�� ui_task.c
   G��G�� sensor_task.c
   G��G�� logging_task.c
```

---

## 19. FreeRTOS Interrupt Handler Requirements (Cortex-M4)

FreeRTOS on Cortex-M4F requires all three exception handlers to be explicitly wired. Missing
any one of them silently prevents the scheduler from running:

```c
// stm32f4xx_it.c

#if defined(USE_FREERTOS)
// vPortSVCHandler sets up the first task's stack frame on scheduler start
void SVC_Handler(void)    { vPortSVCHandler(); }
// xPortPendSVHandler performs context switches between tasks
void PendSV_Handler(void) { xPortPendSVHandler(); }
#endif

void SysTick_Handler(void) {
#if defined(USE_FREERTOS)
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
#endif
    HAL_IncTick();
}
```

**Critical:** if `SVC_Handler` or `PendSV_Handler` are absent or incorrectly named,
`vTaskStartScheduler()` will return without ever running a task. This is a common and silent
failure G�� the application appears to do nothing after reset.

---

## 20. STM32 HAL USE_RTOS Constraint

The STM32F4 HAL header `stm32f4xx_hal_def.h` contains:

```c
#error "USE_RTOS should be 0 in the current HAL release"
```

This fires if `USE_RTOS` is non-zero in `stm32f4xx_hal_conf.h`. The HAL's internal timebase
does not support RTOS-aware scheduling in this release. Keep `USE_RTOS` permanently at `0U`
regardless of whether FreeRTOS is in use G�� all RTOS interaction happens above the HAL through
the OSAL layer.

```c
// boards/<your_board>/include/stm32f4xx_hal_conf.h
#define USE_RTOS    0U    /* must remain 0U even with FreeRTOS enabled */
```

---

## 21. FreeRTOSConfig.h Isolation Rule

`FreeRTOSConfig.h` must never include `stm32f4xx.h` or any CMSIS device header. The FreeRTOS
kernel compiles as a separate CMake target; pulling in device headers through config forces
CMSIS/HAL include paths onto the kernel target and creates cascade build failures.

Instead, hardcode MCU-specific constants:

```c
// rtos/freertos/FreeRTOSConfig.h
#define configCPU_CLOCK_HZ                  100000000UL
#define configTICK_RATE_HZ                  1000
#define configTOTAL_HEAP_SIZE               (12 * 1024)
#define configPRIO_BITS                     4     /* STM32F4: __NVIC_PRIO_BITS */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  5

/* Do NOT add:                                                      */
/* #include "stm32f4xx.h"   <- breaks osal/ and kernel compilation  */
```

---

## 22. Service Layer Split

Once FreeRTOS is present, split services into two kinds:

- **Pure services** (OS-agnostic): logging, parsing, protocol framing, configuration G�� these
  should not depend on osal/ and remain unit-testable on host.
- **Runtime services** (RTOS-aware): worker tasks, message routing, deferred jobs, timers G��
  these depend on osal/ but not directly on FreeRTOS.

---

## 23. Summary

This architecture enables:

- Clean board portability
- Multi-product reuse
- Controlled HAL/LL usage
- Scalable firmware development
- RTOS integration without RTOS sprawl
- Compile-time switching between bare-metal and FreeRTOS builds

The key principle remains: **STM32CubeF4 and FreeRTOS are implementation details G�� not your
architecture.**

---

End of Application Note

---

# Revision Addendum: FreeRTOS + Console Infrastructure (Main-Consistent)

This addendum is intended to be merged into the existing gist
`stm32f4_repo_architecture_application_note.md` so it reflects the current
`main` branch implementation in this repository.

## A. Updated Layered Architecture

### Bare-metal

```text
Application (superloop)
    ->
Services
    ->
Drivers
    ->
OSAL (baremetal backend)
    ->
Platform (STM32F4 abstraction)
    ->
Board (hardware config)
    ->
STM32CubeF4 + CMSIS
```

### With FreeRTOS

```text
Application tasks
    ->
Services
    ->
Drivers
    ->
OSAL (freertos backend)
    ->
Platform (STM32F4 abstraction)
    ->
Board (hardware config)
    ->
STM32CubeF4 + CMSIS + FreeRTOS kernel
```

### Console / Diagnostics path

```text
app + interrupts + RTOS hooks
    ->
console (logging, command parser, fault record)
    ->
platform_uart / platform_usb_cdc
    ->
HAL + board wiring
```

The console is a dedicated infrastructure module (top-level `console/`) and is
linked by `app` and RTOS hook code. It is not implemented as an application
service.

## B. Updated Repository Structure (Current Main)

```text
repo/
|- app/
|  |- app_blinky.c
|  |- app_blinky.h
|  |- app_freertos.c
|  `- app_freertos.h
|- console/
|  |- include/console/console.h
|  `- src/console.c
|- drivers/
|- osal/
|  |- include/osal/
|  |  |- delay.h
|  |  `- task.h
|  `- src/
|     |- baremetal/
|     |  |- osal_delay_baremetal.c
|     |  `- osal_task_baremetal.c
|     `- freertos/
|        |- osal_delay_freertos.c
|        `- osal_task_freertos.c
|- platform/
|- rtos/
|  `- freertos/
|     |- FreeRTOSConfig.h
|     `- freertos_hooks.c
|- projects/blackpill_f411ce_blinky/
|  |- main.c
|  `- stm32f4xx_it.c
`- boards/blackpill_f411ce/
```

## C. CMake Wiring (Main-Consistent)

### Root CMake

```cmake
option(STM32_USE_FREERTOS "Enable FreeRTOS-oriented startup path" OFF)

if(STM32_USE_FREERTOS)
    add_compile_definitions(USE_FREERTOS=1)
endif()

add_subdirectory(platform)
add_subdirectory(rtos)
add_subdirectory(osal)
add_subdirectory(console)
```

### OSAL backend selection

```cmake
# osal/CMakeLists.txt
if(STM32_USE_FREERTOS)
    add_library(osal STATIC
        src/freertos/osal_delay_freertos.c
        src/freertos/osal_task_freertos.c
    )
    target_link_libraries(osal PUBLIC platform_stm32f4_hal freertos_kernel)
else()
    add_library(osal STATIC
        src/baremetal/osal_delay_baremetal.c
        src/baremetal/osal_task_baremetal.c
    )
    target_link_libraries(osal PUBLIC platform_stm32f4_hal)
endif()

target_include_directories(osal PUBLIC include)
```

### FreeRTOS kernel target

```cmake
# rtos/CMakeLists.txt
if(STM32_USE_FREERTOS)
    add_library(freertos_kernel STATIC
        ${FREERTOS_DIR}/croutine.c
        ${FREERTOS_DIR}/event_groups.c
        ${FREERTOS_DIR}/list.c
        ${FREERTOS_DIR}/queue.c
        ${FREERTOS_DIR}/stream_buffer.c
        ${FREERTOS_DIR}/tasks.c
        ${FREERTOS_DIR}/timers.c
        ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
        ${FREERTOS_DIR}/portable/MemMang/heap_4.c
        freertos/freertos_hooks.c
    )
endif()
```

### Console target

```cmake
# console/CMakeLists.txt
add_library(console STATIC src/console.c)
target_include_directories(console PUBLIC include)
target_link_libraries(console PUBLIC platform_stm32f4_hal)

if(STM32_USE_FREERTOS)
    target_link_libraries(console PUBLIC freertos_kernel)
endif()
```

### App linkage

```cmake
# app/CMakeLists.txt
target_link_libraries(app PUBLIC services osal console)
```

## D. Application Startup Model (Main-Consistent)

In `projects/blackpill_f411ce_blinky/main.c`:

- Initialize platform and board.
- Initialize console unconditionally (`console_init()`).
- If `USE_FREERTOS` is set:
  - call `app_rtos_init()`
  - call `osal_scheduler_start()`
- Else run superloop with both:
  - `console_poll()`
  - `app_blinky_run()`

This keeps both runtime modes functionally aligned while preserving a thin main.

## E. FreeRTOS Task Composition in App Layer

Current `app/app_freertos.c` creates:

- `blinky` task: calls `app_blinky_init()` then loops `app_blinky_run()`.
- `console` task: loops `console_poll()`.

Both tasks are created through `osal_task_create()`, not direct `xTaskCreate()`
outside OSAL wrappers.

## F. Interrupt + Hook Integration for Console Diagnostics

### ISR side

`projects/blackpill_f411ce_blinky/stm32f4xx_it.c`:

- Wires FreeRTOS handlers when `USE_FREERTOS`:
  - `SVC_Handler -> vPortSVCHandler`
  - `PendSV_Handler -> xPortPendSVHandler`
  - `SysTick_Handler` forwards to `xPortSysTickHandler` once scheduler starts
- Routes transport interrupts:
  - `USART1_IRQHandler -> platform_uart_irq_handler(...)`
  - `OTG_FS_IRQHandler -> platform_usb_cdc_irq_handler()`
- Hard fault path records and emits panic diagnostics through console.

### RTOS hooks side

`rtos/freertos/freertos_hooks.c`:

- `vApplicationMallocFailedHook()` logs and traps.
- `vApplicationStackOverflowHook()` logs task context and traps.

### Assertion path

`rtos/freertos/FreeRTOSConfig.h`:

- `configASSERT(...)` is wired to `console_assert_failed(...)`.

This creates one consistent fault reporting pathway for kernel/runtime failures.

## G. Console Module Responsibility and API

`console/include/console/console.h` currently exposes:

- init and polling: `console_init()`, `console_poll()`
- log level control
- leveled logging macros (`CONSOLE_LOGE/W/I/D`)
- panic logging
- fault record capture and retrieval
- assertion failure handler

`console/src/console.c` currently provides:

- line-oriented command interface (`help`, `version`, `uptime`, `log level`,
  `fault last`)
- dual transport behavior:
  - bare-metal path uses UART write/read flow
  - FreeRTOS path uses stream buffer fed by UART IRQ callback and CDC output
- bounded message formatting and drop counting

## H. Dependency Rules (Revised)

### Runtime dependencies (effective current policy)

```text
app -> services -> drivers -> platform -> board -> vendor
app -> osal
app -> console
console -> platform (+ freertos_kernel when enabled)
osal(freertos backend only) -> freertos_kernel
rtos hooks -> console
```

### Inclusion policy

- App/services/drivers do not include FreeRTOS headers directly.
- FreeRTOS headers are isolated to:
  - `osal/src/freertos/`
  - `rtos/freertos/`
  - selected integration points (`console/src/console.c`, interrupt file) that
    are explicitly guarded by `USE_FREERTOS`.

## I. Notes to Replace/Amend Existing Gist Statements

1. If the gist states diagnostics should live in a top-level `diag/` module,
   revise to match implemented naming (`console/`) while keeping the same
   architecture intent.
2. If the gist implies command shell is deferred, add that current `main`
   already includes a minimal production command surface as infrastructure.
3. Keep `USE_RTOS` in STM32 HAL config at `0U`; FreeRTOS enablement is controlled
   by `STM32_USE_FREERTOS` and `USE_FREERTOS` compile definition.
4. Keep the `FreeRTOSConfig.h` isolation rule (no device header include).

## J. Suggested New Section Title in the Gist

Add a dedicated section after FreeRTOS integration content:

### "Console Infrastructure and Fault Telemetry"

Position it as first-class infrastructure alongside OSAL/RTOS integration, with
clear rationale:

- deterministic runtime diagnostics
- unified panic/fault path
- same API across bare-metal and FreeRTOS builds
- command interface for field triage without application coupling
