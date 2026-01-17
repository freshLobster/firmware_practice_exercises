# Exercise 1 — Bare-Metal STM32F4 Firmware (From Scratch)

This repository contains a learn-by-building firmware exercise: bring up a minimal bare-metal project for an STM32F4-class MCU without HAL, CubeMX, or an RTOS.

Target device: STM32F407 (Cortex-M4F)  
Primary validation environment: Renode

The goal is to end with a `firmware.elf` that toggles GPIO PD12 at a steady interval using a SysTick-driven tick counter and a wrap-safe scheduler.

## Who this is for

This is for people who want to learn professional-level firmware bringup by doing the real work:
- building a cross-compiled toolchain pipeline
- writing a linker script
- implementing startup/reset code
- defining minimal register maps
- verifying memory placement and runtime behavior with real tools

There are no training wheels. You are expected to read datasheets and reason about what the MCU is doing.

## Repository layout

Work happens in `exercise_1/`.

```
exercise_1/
├─ CMakeLists.txt
├─ cmake/
│  └─ arm-none-eabi.cmake
├─ include/
│  └─ stm32f4xx_min.h
├─ boards/
│  └─ stm32f4disco/
│     └─ linker.ld
├─ src/
│  ├─ startup_stm32f407xx.s
│  ├─ system_stm32f4xx.c
│  └─ main.c
└─ scripts/
   └─ run-stm32f4.resc
```

## Tooling prerequisites

Required:
- ARM GNU toolchain: `arm-none-eabi-gcc`
- CMake (3.24+ recommended)
- Ninja
- Renode

Useful:
- `arm-none-eabi-objdump`
- `arm-none-eabi-readelf`

## Build and run quickstart

From repo root:

```bash
cmake -S exercise_1 -B exercise_1/build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=exercise_1/cmake/arm-none-eabi.cmake

cmake --build exercise_1/build
```

Then run in Renode using the provided script (see `scripts/run-stm32f4.resc`).

## Part 1 — Cross compilation with CMake

### 1.1 Toolchain file

File: `exercise_1/cmake/arm-none-eabi.cmake`

This must:
- set a bare-metal target system (Generic)
- use ARM cross compilers
- prevent CMake from pulling host includes/libs

Minimal structure:

```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_GCC arm-none-eabi-gcc REQUIRED)
find_program(ARM_GPP arm-none-eabi-g++ REQUIRED)
find_program(ARM_GAS arm-none-eabi-gcc REQUIRED)

set(CMAKE_C_COMPILER ${ARM_GCC})
set(CMAKE_CXX_COMPILER ${ARM_GPP})
set(CMAKE_ASM_COMPILER ${ARM_GAS})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

### 1.2 Top-level CMakeLists

File: `exercise_1/CMakeLists.txt`

Important: the toolchain must be set before `project()`.

Use Cortex-M4F flags (hard-float) and link with the provided linker script.

Example structure:

```cmake
cmake_minimum_required(VERSION 3.24)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi.cmake"
    CACHE FILEPATH "" FORCE)

project(firmware C ASM)

set(MCU_FLAGS
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=hard
)

add_executable(firmware.elf
  src/startup_stm32f407xx.s
  src/system_stm32f4xx.c
  src/main.c
)

target_include_directories(firmware.elf PRIVATE
  include
  boards/stm32f4disco
)

target_compile_options(firmware.elf PRIVATE
  ${MCU_FLAGS}
  -ffunction-sections
  -fdata-sections
  -Wall -Wextra
)

target_link_options(firmware.elf PRIVATE
  ${MCU_FLAGS}
  -Wl,--gc-sections
  -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map
  -T${CMAKE_SOURCE_DIR}/boards/stm32f4disco/linker.ld
)
```

## Part 2 — Minimal register definitions (no CMSIS/HAL)

File: `exercise_1/include/stm32f4xx_min.h`

Define only what you use. At minimum:
- RCC (to enable the GPIO peripheral clock)
- GPIO for port D (mode config + BSRR)
- SysTick (CTRL/LOAD/VAL)
- SCB (priority registers, and CPACR for FPU enable if you do it here)

Use `volatile uint32_t` fields for registers. Keep base addresses correct.

Notes:
- This is MCU- and core-specific work. Do not assume these definitions are universal across Cortex-M variants.
- You are responsible for verifying base addresses and bit positions.

## Part 3 — Linker script (memory map and sections)

File: `exercise_1/boards/stm32f4disco/linker.ld`

Set correct FLASH/RAM regions for STM32F407-class parts:

- FLASH origin: `0x08000000`
- RAM origin: `0x20000000`

Your linker script must:
- place the vector table at the start of FLASH
- place `.text`/`.rodata` in FLASH
- place `.data` in RAM, but load it from FLASH (VMA in RAM, LMA in FLASH)
- place `.bss` in RAM

Critical point:
- `.data` must have a real FLASH load address. If you accidentally give it a load address of `0x00000000`, your startup copy will be wrong and the program will not behave.

You must define the symbols used by your startup code, typically including:
- `_estack`
- `_sidata`, `_sdata`, `_edata`
- `_sbss`, `_ebss`

## Part 4 — Startup code (vector table and Reset_Handler)

File: `exercise_1/src/startup_stm32f407xx.s`

Provide:
- vector table (stack pointer + handlers)
- `Reset_Handler`
- weak aliases for other interrupts to a `Default_Handler`

The `Reset_Handler` must:
1. call `SystemInit`
2. copy `.data` from its FLASH load location to its RAM run location
3. zero `.bss`
4. call `main`
5. loop forever if `main` returns

If you see "missing entry symbol" or you do not reach `main`, treat startup as the first suspect.

## Part 5 — System init (clock and FPU)

File: `exercise_1/src/system_stm32f4xx.c`

Keep clocks simple. A common minimal path is:
- leave the default HSI clock configuration
- set/maintain `SystemCoreClock` appropriately

Enable the FPU on Cortex-M4F if you are using it:
- CP10 and CP11 full access in SCB->CPACR

If you configure SysTick from `SystemCoreClock`, you must ensure it reflects reality.

## Part 6 — SysTick tick counter and GPIO blink

File: `exercise_1/src/main.c`

### 6.1 SysTick tick

Maintain a monotonically increasing tick in the SysTick ISR:

```c
volatile uint32_t sys_tick;

void SysTick_Handler(void)
{
    sys_tick++;
}
```

Configure SysTick for 1 kHz:

```c
SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
SysTick->VAL  = 0;
SysTick->CTRL = (1U << 0) | (1U << 1) | (1U << 2); // ENABLE | TICKINT | CLKSOURCE
```

### 6.2 GPIO PD12 setup and atomic toggling

Enable the GPIO clock via RCC, configure PD12 as an output, then set/reset using BSRR:

```c
GPIOD->BSRR = (1U << 12);        // set PD12
GPIOD->BSRR = (1U << (12 + 16)); // reset PD12
```

Do not toggle by read-modify-write on ODR unless you know exactly why it is safe for your use case.

### 6.3 Wrap-safe scheduling

Do not write timing code that fails on tick overflow. Use signed subtraction:

```c
if ((int32_t)(sys_tick - next_toggle) >= 0)
{
    next_toggle += period_ms;
    // toggle
}
```

## Part 7 — Verify memory placement

After building, inspect sections:

```bash
arm-none-eabi-objdump -h exercise_1/build/firmware.elf
```

Confirm:
- `.isr_vector` begins at `0x08000000`
- `.text` is in FLASH
- `.data` VMA is in RAM and LMA is in FLASH (not `0x00000000`)
- `.bss` is in RAM

If these are not true, fix the linker script and startup before debugging anything else.

## Part 8 — Run in Renode

Use the script:

- `exercise_1/scripts/run-stm32f4.resc`

A typical Renode script:
- creates the machine
- loads an STM32F4 Discovery platform description
- loads the ELF into memory
- starts execution

If you want to validate GPIO activity, enable peripheral access logging in Renode and observe writes to the GPIOD registers (especially BSRR).

## Expected end state

You have:
- a clean cross-compiled build that produces `firmware.elf`
- correct FLASH/RAM section placement with a functioning startup path
- SysTick interrupt running at 1 kHz and incrementing a tick counter
- PD12 toggling at a steady interval using wrap-safe scheduling
- the firmware running cleanly under Renode without fault loops
