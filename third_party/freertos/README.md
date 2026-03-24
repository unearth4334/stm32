# FreeRTOS third-party placeholder

Populate this path with the official FreeRTOS kernel source.

Expected layout:

```
third_party/freertos/
└── FreeRTOS-Kernel/
    ├── include/
    ├── portable/
    │   ├── GCC/ARM_CM4F/
    │   └── MemMang/
    ├── tasks.c
    ├── queue.c
    ├── list.c
    ├── timers.c
    ├── event_groups.c
    └── stream_buffer.c
```

Recommended:

```sh
git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel third_party/freertos/FreeRTOS-Kernel
```
