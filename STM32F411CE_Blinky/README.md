# STM32F411CE Blinky

Modular STM32Cube + PlatformIO blinky project for Black Pill (`blackpill_f411ce`) with clear layering:

- `src/`: app entry and app behavior
- `lib/board/`: board support package (BSP)
- `lib/drivers_led/`: LED driver abstraction
- `lib/drivers_sts40/`: portable STS40 temperature sensor driver abstraction
- `include/`: centralized compile-time configuration headers

## Configuration headers

- `include/project_config.h`: project-wide feature toggles
- `include/board_config.h`: board pin map and blink timing
- `include/driver_config.h`: driver-specific defaults
- `include/sts40_driver_config.h`: STS40 protocol and behavior tuning
- `include/error_config.h`: assert/error handling macros

## STS40 Driver

Implemented from `documentation/HT_DS_Datasheet_STS4x.pdf` (Version 3.0, June 2024):

- Commands: `0xFD` (high), `0xF6` (medium), `0xE0` (low), `0x89` (serial), `0x94` (soft reset)
- Supported I2C addresses: `0x44`, `0x45`, `0x46`
- CRC-8: polynomial `0x31`, init `0xFF`, final XOR `0x00`
- Temperature conversion:
	- `T(°C) = -45 + 175 * raw / 65535`
	- `T(°F) = -49 + 315 * raw / 65535`

Portability approach:

- No HAL dependency in STS40 driver source
- Host MCU integration through callbacks (`i2c_write`, `i2c_read`, `delay_ms`)
- Configurable retries, CRC checking, command constants, timing, and default repeatability through `include/sts40_driver_config.h`

Key API header: `lib/drivers_sts40/include/sts40_driver.h`

## USB Console Interface (STS4x test + config)

The firmware includes a command console designed to run over the board USB port (CDC-ACM style serial).

- Console core: `lib/console/include/console.h` + `lib/console/src/console.c`
- USB port adapter contract: `include/usb_console_port.h`
- App command integration: `src/app_blinky.c`

Implemented console commands:

- `help`
- `status`
- `read c|f|raw`
- `serial`
- `reset soft|gc`
- `config show`
- `config addr <0x44|0x45|0x46>`
- `config rep <low|medium|high>`
- `config crc <on|off>`
- `config retry <count> <delay_ms>`
- `poll on <period_ms>` / `poll off`
- `led on|off|toggle`

The default implementation in `src/usb_console_port.c` is a weak stub. To use real USB console I/O,
override `UsbConsole_ReadByte()` and `UsbConsole_Write()` from your USB CDC transport layer.

## Build

```bash
cd STM32F411CE_Blinky
pio run
```

## Upload via Docker DFU

1. Put board in DFU mode (`BOOT0=1`, reset)
2. Build + upload:

```bash
cd STM32F411CE_Blinky
pio run -t upload
```

`upload` uses `scripts/dfu_upload.sh`, which invokes the container from the workspace root using `../docker-compose.yml`.

The upload script waits for DFU device `0483:df11` before flashing.
Optional tuning:

```bash
DFU_WAIT_SECONDS=30 DFU_RETRY_INTERVAL=1 pio run -t upload
```

Verbose retry diagnostics (prints `dfu-util -l` each poll):

```bash
DFU_VERBOSE=1 DFU_WAIT_SECONDS=30 pio run -t upload
```
