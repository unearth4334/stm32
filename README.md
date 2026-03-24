# STM32F4 Firmware — Redesigned Architecture

> Branch: `redesign/blackpill-f411-architecture`  
> Target board: **WeAct STM32F411CE BlackPill v3.1**  
> Motivated by: [STM32F4 Repo Architecture Application Note](https://gist.github.com/unearth4334/3bc3fd054c4cf47678f7f9a024978de9)

---

## Architecture

```
┌─────────────────────────────────────┐
│              app/                   │  Application logic (blinky, …)
├─────────────────────────────────────┤
│           services/                 │  High-level hardware abstractions
├─────────────────────────────────────┤
│          middleware/                │  Runtime/middleware (FreeRTOS, etc.)
├─────────────────────────────────────┤
│            drivers/                 │  Hardware-agnostic drivers
├─────────────────────────────────────┤
│  platform/include/platform/         │  Abstract platform API (headers)
│  platform/src/stm32f4/hal           │  STM32F4 HAL implementation
│  platform/src/stm32f4/ll            │  STM32F4 LL (performance-critical)
│  platform/src/stm32f4/common        │  clock config, low-level init
├─────────────────────────────────────┤
│   boards/blackpill_f411ce/          │  Pin mappings, board_init()
├─────────────────────────────────────┤
│  third_party/cmsis                  │  Vendor code (not architecture)
│  third_party/stm32cubef4            │
└─────────────────────────────────────┘
```

**Dependency rule (never reverse):**
```
app → services → drivers → platform → board → vendor
```

**RTOS extension (this exploration branch):**
```
app/services → middleware/rtos → platform → board → vendor
```

---

## Repo Structure

```
.
├── app/                        Application layer
├── services/                   Service layer
├── middleware/
│   ├── include/middleware/     RTOS abstraction headers
│   └── rtos/                   RTOS adapter implementation
├── drivers/
│   └── led/                    LED driver (platform GPIO abstraction)
├── platform/
│   ├── include/platform/       Public abstract platform headers
│   └── src/stm32f4/
│       ├── hal/                HAL implementations
│       ├── ll/                 LL implementations (add as needed)
│       └── common/             platform_init, SystemClock_Config
├── boards/
│   └── blackpill_f411ce/       WeAct STM32F411CE pin config + board_init
├── third_party/                Vendor code — see third_party/README.md
│   └── freertos/               FreeRTOS kernel placeholder
├── linker/
│   └── stm32f411.ld            Linker script (512 KB flash, 128 KB SRAM)
├── startup/                    startup_stm32f411xe.s — see startup/README.md
├── tests/                      Host-side unit tests + target integration tests
├── projects/
│   └── blackpill_f411ce_blinky/  main.c for the blinky example
├── cmake/
│   └── arm-none-eabi.cmake     Cross-compilation toolchain file
└── CMakeLists.txt
```

---

## Hardware — BlackPill STM32F411CE

| Resource    | Value                        |
|-------------|------------------------------|
| MCU         | STM32F411CEU6                |
| Flash       | 512 KB @ 0x08000000          |
| SRAM        | 128 KB @ 0x20000000          |
| Max clock   | 100 MHz (PLL from 25 MHz HSE)|
| User LED    | PC13 (active-low)            |
| User button | PA0  (active-high)           |
| Debug       | SWD (PA13/PA14)              |

---

## Getting Started

### 1. Populate `third_party/`

```sh
git submodule add https://github.com/STMicroelectronics/cmsis_core   third_party/cmsis
git submodule add https://github.com/STMicroelectronics/stm32cubef4  third_party/stm32cubef4
git submodule update --init --recursive
```

### 2. Copy startup files

```sh
cp third_party/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s startup/
cp third_party/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c        startup/
```

Uncomment the startup line in `projects/blackpill_f411ce_blinky/CMakeLists.txt`.

### 3. Build

```sh
cmake -B build \
      -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### 3b. Build with FreeRTOS enabled (exploration)

```sh
cmake -B build \
      -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
      -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_FREERTOS=ON
cmake --build build -j$(nproc)
```

Populate `third_party/freertos/FreeRTOS-Kernel` first (see `third_party/freertos/README.md`).

Output: `build/projects/blackpill_f411ce_blinky/firmware.elf` + `.bin` + `.hex`

### 4. Flash (DFU)

```sh
dfu-util -a 0 -s 0x08000000:leave -D build/projects/blackpill_f411ce_blinky/firmware.bin
```

---

## Design Principles (from application note)

- STM32CubeF4 is an **implementation detail**, not the architecture.
- Drivers never include STM32 headers — only platform source files do.
- App and services layers never reference GPIO pins or peripheral instances.
- One linker script per MCU variant.
- HAL for general peripherals; LL for performance-critical / ISR paths.
