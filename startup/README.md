# startup

Contains startup assembly files and the system initialisation source for each supported MCU.

For STM32F411CE, copy `startup_stm32f411xe.s` from:

```
third_party/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s
```

And `system_stm32f4xx.c` from:

```
third_party/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
```

Place them here once `third_party/` is populated. The project CMakeLists will reference them automatically.
