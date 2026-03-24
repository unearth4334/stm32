# tests

## Host-side (native)

Unit-test the application and driver layers using a mock platform.

```
tests/
├── host/
│   ├── mock/
│   │   └── platform_gpio_mock.c   # stub platform/gpio.h for host
│   ├── test_led_driver.c
│   ├── test_led_service.c
│   └── test_app_blinky.c
└── CMakeLists.txt
```

Run with:
```sh
cmake -B build/host
cmake --build build/host
ctest --test-dir build/host
```

## Target (on-device)

Integration tests for platform, timing, IRQs, and DMA are run directly on hardware.  
Add them under `tests/target/` using your preferred framework (Unity, Ceedling, etc.).
