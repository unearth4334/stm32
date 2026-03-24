# third_party

This directory holds vendor-managed code that is **not part of this project's architecture**. It is treated as an opaque implementation detail by the platform layer — nothing above `platform/` should include these headers.

## Required contents

```
third_party/
├── cmsis/
│   ├── Include/              # CMSIS core headers (core_cm4.h, etc.)
│   └── Device/
│       └── ST/
│           └── STM32F4xx/
│               └── Include/  # stm32f411xe.h, system_stm32f4xx.h
└── stm32cubef4/
    └── Drivers/
        └── STM32F4xx_HAL_Driver/
            ├── Inc/
            └── Src/
```

## How to populate

**Option A — Git submodule (recommended)**
```sh
git submodule add https://github.com/STMicroelectronics/cmsis_core        third_party/cmsis
git submodule add https://github.com/STMicroelectronics/stm32cubef4       third_party/stm32cubef4
```

**Option B — Manual download**  
Download STM32CubeF4 from [st.com](https://www.st.com/en/embedded-software/stm32cubef4.html) and copy the `Drivers/` tree here.

This directory is listed in `.gitignore` — do not commit vendor sources.
