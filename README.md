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

---

## Repo Structure

```
.
├── app/                        Application layer
├── services/                   Service layer
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

Output: `build/projects/blackpill_f411ce_blinky/firmware.elf` + `.bin` + `.hex`

### 4. Flash (DFU)

```sh
dfu-util -a 0 -s 0x08000000:leave -D build/projects/blackpill_f411ce_blinky/firmware.bin
```

### 5. Monitor ADS7822 Output

The ADS7822 integration logs periodic samples through the console layer using lines like:

```text
[1234] I/ads7822: sample=2048
```

Use the host-side monitor script to watch those lines on the board's serial console and convert raw samples into volts:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/monitor_adc.ps1 -Port COM7
```

Common options:

```powershell
# Auto-select a likely STM32 serial port when possible
powershell -ExecutionPolicy Bypass -File scripts/monitor_adc.ps1

# Save parsed samples to CSV while monitoring
powershell -ExecutionPolicy Bypass -File scripts/monitor_adc.ps1 -Port COM7 -CsvPath adc.csv

# Print every console line in addition to parsed ADC samples
powershell -ExecutionPolicy Bypass -File scripts/monitor_adc.ps1 -Port COM7 -PassThru
```

Notes:

- FreeRTOS builds emit the console over USB CDC, so the board should appear as a COM port on the host.
- Bare-metal builds use the debug UART on USART1 at 115200 baud.
- The script defaults to a 3.0 V ADC reference to match the current board wiring.

---

## Design Principles (from application note)

- STM32CubeF4 is an **implementation detail**, not the architecture.
- Drivers never include STM32 headers — only platform source files do.
- App and services layers never reference GPIO pins or peripheral instances.
- One linker script per MCU variant.
- HAL for general peripherals; LL for performance-critical / ISR paths.
