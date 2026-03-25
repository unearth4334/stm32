# Console Interface Stream Research

## Scope

Target: STM32F411 BlackPill firmware with optional FreeRTOS, existing layered architecture, and a new console stream that can support:

- interactive command handling
- general diagnostics/logging
- crash and fault traceback
- future trace/event streaming

This note focuses on battle-tested patterns rather than a one-off `printf` retarget.

## Current Repo Constraints

- The project already reserves a debug UART in `board.h` as `BOARD_DEBUG_UART` at `115200`.
- There is no public UART abstraction yet in `platform/include/platform`.
- The existing layer rule is `app -> services -> drivers -> platform -> board -> vendor`.
- Logging and fault reporting will need to be callable from drivers, services, app, RTOS hooks, and exception handlers.
- `configASSERT()` and the FreeRTOS hooks currently halt in infinite loops without any structured crash output.

The last point matters: a console subsystem placed in `services/` would be architecturally wrong because lower layers and fault handlers need access to it.

## Battle-Tested Patterns

### 1. Frontend + Multiple Backends

This is the strongest pattern across mature embedded stacks.

Observed in:

- Zephyr logging: frontend/core/backends, deferred logging, panic mode, per-backend filtering.
- ESP-IDF logging: common API, per-module tags, runtime level control, early/constrained logging variants, pluggable output sink.

What to copy:

- one logging API used everywhere
- transport-specific backends hidden behind a small stream interface
- normal mode can be buffered/deferred
- panic mode must switch to synchronous, minimal, blocking-safe output
- compile-time max level plus runtime current level
- per-module tag or source name
- hexdump support and dropped-message accounting

What not to copy wholesale:

- heavy macro/config systems
- dynamic registries or large formatting stacks unless proven necessary

### 2. Stream Transport Separate from Command Shell

Mature systems do not conflate byte transport with command parsing.

Observed in:

- Zephyr shell: shell core with UART, RTT, USB, Telnet backends.
- FreeRTOS+CLI demos: command interpreter independent from UART or UDP transport.

What to copy:

- a byte stream abstraction for tx/rx
- a shell/parser layer on top of the stream
- the ability to run logs and commands over separate channels or separate streams

This separation is important if you later want:

- UART shell + RTT logs
- UART logs + USB CDC shell
- host scripting against a quiet command channel while logs continue elsewhere

### 3. Dedicated Panic/Fault Capture Path

Crash handling should not depend on the normal scheduler-safe logging path.

Observed in:

- Memfault Cortex-M hardfault workflow: capture stacked registers, CFSR/HFSR/BFAR/MMFAR, symbolicate offline.
- Zephyr logging panic mode: flush buffers and switch backends into synchronous behavior.

What to copy:

- HardFault shim in assembly chooses MSP or PSP based on `EXC_RETURN`
- pass stacked frame into a C fault handler
- capture at minimum:
  - `r0-r3`, `r12`, `lr`, `pc`, `xpsr`
  - `CFSR`, `HFSR`, `DFSR`, `BFAR`, `MMFAR`, `AFSR`
  - active stack pointer
  - current task name if FreeRTOS is running
- write a compact crash record to retained RAM or `.noinit`
- emit a short panic line on the panic transport if safe
- do full symbolication offline from the ELF and map file

Important constraint: on Cortex-M, "traceback" in production is usually a captured fault frame plus offline symbolication, not a reliable full stack unwind. That is the battle-tested approach.

### 4. Buffered Normal Path, Minimal ISR Path

Observed in:

- Zephyr deferred logging
- ESP-IDF normal vs early/constrained log macros
- SEGGER RTT blocking vs non-blocking channels

What to copy:

- task/thread context: enqueue formatted or preformatted log records
- ISR/fault context: only use a small, lock-free or interrupt-safe path
- avoid calling full `printf` from fault handlers or high-rate ISRs

### 5. Small Static Command Registry

Observed in:

- FreeRTOS+CLI: static command definitions registered into a list, single interpreter, repeated processing until command output is complete

What to copy:

- fixed command table or explicit registration API
- text commands with help strings
- parser state owned by one console session

What to avoid:

- building a large interactive shell first
- making the command engine re-entrant before there is a real need

For this repo, FreeRTOS+CLI is a good reference model, not necessarily a library dependency that must be imported unchanged.

## Transport Options

### A. UART with Interrupt or DMA Ring Buffers

Pros:

- universally deployable
- works without a debugger
- suitable for production/operator console access
- compatible with USB-UART adapters and automation

Cons:

- significantly slower than memory-backed debug transports
- easy to stall if done synchronously
- requires careful handling to avoid log interleaving and backpressure problems

Recommendation:

- make this the baseline production transport
- use IRQ or DMA-backed ring buffers, not blocking polled writes in the normal path

### B. SEGGER RTT

Pros:

- very fast on Cortex-M
- bidirectional
- no extra pins
- battle-tested for interactive debug I/O
- supports separate up/down channels and blocking or non-blocking behavior

Cons:

- requires J-Link tooling, so it is primarily a development transport
- not a general field-service interface unless your workflow standardizes on J-Link

Recommendation:

- make this the preferred developer transport if the team uses J-Link
- excellent candidate for panic output and high-volume logs
- particularly useful if UART is needed for the application or for customer-facing access

### C. ITM/SWO

Pros:

- fast and common in ARM debug workflows
- lower code footprint than a full UART stack

Cons:

- more tooling variability than RTT
- usually output-only for practical purposes
- weaker fit for an interactive shell

Recommendation:

- useful as an optional log backend, not as the primary console design center

## Recommendation for This Repo

### Short Version

Implement a new low-level diagnostics subsystem with:

1. a transport-agnostic byte stream interface
2. a logger frontend with module tags and log levels
3. a panic/fault path that bypasses the normal logger buffering
4. a small command console layered on the stream, initially UART-backed
5. optional SEGGER RTT backend for development

### Why This Fits the Existing Architecture

Do not place this in `services/`.

Drivers, RTOS hooks, and fault handlers all need access. The cleanest fit is a new top-level library parallel to `osal/` and `rtos/`, for example:

- `diag/`
- or `console/`

I would prefer `diag/` because the scope is larger than a shell. It includes logging, panic capture, and future trace/event streaming.

Suggested dependency direction:

- `app -> services -> drivers`
- `app/services/drivers -> diag`
- `diag -> osal`
- `diag -> platform`
- `platform -> board -> vendor`

That preserves the existing hardware abstraction while giving lower layers a legal path to emit diagnostics.

## Proposed Module Split

### `diag/include/diag/log.h`

Public logging API.

Minimum API shape:

- `diag_log_init()`
- `diag_log_set_level()`
- `diag_log_write(level, tag, fmt, ...)`
- `diag_log_hexdump(level, tag, data, len, msg)`
- `diag_log_panic_flush()`

Optional macro layer:

- `LOGE(tag, ...)`
- `LOGW(tag, ...)`
- `LOGI(tag, ...)`
- `LOGD(tag, ...)`

### `diag/include/diag/stream.h`

Abstract byte stream interface.

Minimum responsibilities:

- write bytes
- poll/read bytes
- query space or drop status
- enter panic mode

Backends should implement:

- UART stream
- RTT stream later

### `diag/include/diag/console.h`

Command console running on a chosen stream.

Minimum responsibilities:

- line editing at the simple level only
- command registration
- help output
- dispatch callbacks

Start small. Zephyr-shell-level UX is not required for the first pass.

### `diag/include/diag/fault.h`

Fault capture and retained crash record API.

Minimum responsibilities:

- register a retained crash record buffer
- expose `diag_fault_last_record()`
- stringify fault status fields for host-readable output if needed

## Implementation Strategy

### Phase 1. Transport Foundation

- add `platform/uart.h` and `platform_uart.c`
- start with UART tx/rx ring buffers
- expose a thin opaque handle API like `platform_i2c`
- wire the chosen UART instance from `board.h`

This is the minimum needed to avoid tying the rest of the design to HAL UART calls.

### Phase 2. Logger Core

- add a small static log record ring buffer
- compile-time max level, runtime current level
- tag strings per module
- synchronous fallback when scheduler is not running
- message drop counter

If FreeRTOS is enabled, a dedicated low-priority drain task is reasonable. In bare-metal mode, draining can happen from the main loop or immediately depending on the selected mode.

### Phase 3. Panic and Fault Path

- replace the current HardFault path with an assembly shim + C decoder
- update `configASSERT()` and FreeRTOS hooks to call panic logging helpers before halting
- emit one compact panic line and save a retained crash record

### Phase 4. Command Console

- implement a tiny static command registry
- initial commands should focus on system introspection:
  - `help`
  - `version`
  - `uptime`
  - `tasks` when FreeRTOS is enabled
  - `heap` when relevant
  - `fault last`
  - `log level <tag|*> <level>`
- process input in a dedicated task under FreeRTOS and in the main loop for bare-metal builds

### Phase 5. RTT Backend

- add optional SEGGER RTT backend
- allow separate channels for logs vs shell
- use non-blocking mode for high-rate logs
- optionally use a blocking or trimmed channel for shell traffic depending on behavior goals

## Decision Matrix

### Best Development Setup

- logs: RTT
- commands: RTT or UART
- faults: RTT panic line + retained crash record

### Best Production/Field Setup

- logs: UART at controlled verbosity
- commands: UART or USB CDC in the future
- faults: retained crash record, optional one-line panic banner if transport is alive

### Best First Increment for This Branch

- transport abstraction
- UART backend
- minimal logger
- fault record capture

Do not start with a full shell. Start with the logging and fault plumbing because traceback requirements will shape the architecture more than the command parser will.

## Risks to Avoid

- Retargeting `_write()` or `printf()` globally as the only design. This becomes hard to control in ISR, startup, and panic contexts.
- Placing console code in `services/`, which would block lower-layer diagnostics.
- Using one blocking transport path for everything.
- Depending on `printf` in fault handlers.
- Mixing shell output and logs on one stream without serialization rules.
- Assuming full stack unwinding will be robust on shipped Cortex-M firmware.

## Recommended Branch Outcome

If this branch moves from research into implementation, the first PR should establish the architecture only:

- `platform` UART abstraction
- `diag` library skeleton
- UART stream backend
- fault record struct and HardFault shim
- FreeRTOS hook/assert integration

Then add the command console in a second PR.

That split reduces risk and makes the traceback path reviewable on its own.