# STM32 DFU Docker setup (Black Pill)

This container lets you use `dfu-util` from Docker to flash an STM32 Black Pill over the built-in USB DFU bootloader.

## Files

- `Dockerfile`: Ubuntu image with `dfu-util`
- `docker-compose.yml`: mounts project folder and USB bus

## 1) Build image

```bash
docker compose build
```

## 2) Put Black Pill into DFU mode

Typical STM32 bootloader sequence:

1. Set `BOOT0` to `1` (high)
2. Press `NRST` (reset)
3. Board enumerates as ST DFU device (`0483:df11`)

## 3) Start container shell

```bash
docker compose run --rm stm32-dfu
```

Inside the container:

```bash
dfu-util -l
```

You should see the STM32 DFU device listed.

## 4) Flash firmware

Assuming your binary is `build/firmware.bin`:

```bash
dfu-util -a 0 -s 0x08000000:leave -D build/firmware.bin
```

## Optional: one-shot flash command

From host:

```bash
docker compose run --rm stm32-dfu \
  dfu-util -a 0 -s 0x08000000:leave -D build/firmware.bin
```

## Linux notes (permissions)

If `dfu-util -l` cannot access USB:

- Try running Docker with elevated privileges (`sudo docker ...`) if your user is not in the `docker` group.
- Ensure no host process is claiming the device.
- As a fallback, add `privileged: true` to `stm32-dfu` in `docker-compose.yml`.

## Sanity check

Inside container:

```bash
lsusb | grep -i 'STMicroelectronics\|DFU\|0483:df11'
```

If detected, DFU flashing from container is ready.
