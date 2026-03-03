# STM32F411CE Blinky

Modular STM32Cube + PlatformIO blinky project for Black Pill (`blackpill_f411ce`) with clear layering:

- `src/`: app entry and app behavior
- `lib/board/`: board support package (BSP)
- `lib/drivers_led/`: LED driver abstraction
- `include/`: centralized compile-time configuration headers

## Configuration headers

- `include/project_config.h`: project-wide feature toggles
- `include/board_config.h`: board pin map and blink timing
- `include/driver_config.h`: driver-specific defaults
- `include/error_config.h`: assert/error handling macros

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
