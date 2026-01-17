# Exercise 1 — Blinking an LED with no MCU and no LED
 The LOOOOONG way
(Bare-Metal STM32F4 firmware from scratch with no helpers)

This repository contains a learn-by-building firmware exercise: bring up a minimal bare-metal project for an STM32F4-class microcontroller.
This exercise was designed to be done at a professional level with no hobbyist tools (Arduino IDE, PlatformIO, etc.)
==There is no need to own or posess the target hardware.== Anyone can complete this exercise with only a text editor, the gcc compiler, and Renode. 
Since in a professional environment you may or may not have access to the target hardware, we will simulate it with Renode and confirm behavior by reading registers and tracking function calls.
This is the first of multiple exercises that progressively get more challenging.
I created these to practice and maintain my own embedded skills during a period in which I am not currently employed. Through building out lessons and explaining the topics I really get a better and deeper understanding of it myself.
My hope is that others may benefit from these exercises as well.

If you notice any mistakes or have questions please feel free to email me: aidanseine@gmail.com

### For this project I used:

- VS Code with the following extensions:
- - C/C++ Extension Pack
- - Arm Assembly Support
- - CMake Tools
- - Python
- - STM32Cube CMake Support
- - STMCube Core

- Renode


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

**If you get stuck, refer to the already completed exercise_1 directory included in this repository.**

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
In embedded, your code doesn’t run on your computer’s CPU. It runs on a completely different processor with a different instruction set, memory map, ABI, and runtime environment (usually no OS). That means you can’t just compile with your normal compiler and hit “Run”. You have to cross-compile: build code on your machine that targets the MCU. This is one of the big “embedded vs normal software” shifts: you’re building an executable that has no operating system underneath it, so things like startup code, stack location, and how memory is initialized become your responsibility.

CMake is used here not because it’s trendy, but because it’s a repeatable way to express a professional embedded build pipeline and to separate host tooling from target tooling cleanly.


### 1.1 Toolchain file

A CMake toolchain file is basically how you tell CMake: “Stop thinking you’re building for my laptop.” By default, CMake assumes you’re compiling for the machine it’s running on, and it will try to locate system libraries, default include paths, and platform-specific build behavior. That’s poison for embedded. In firmware, there usually is no libc provided by an OS, no system dynamic linker, no filesystem, and no standard runtime environment. So the toolchain file forces CMake into “bare-metal mode”.

The important concept is that `arm-none-eabi-gcc` is not “just another gcc”. It’s a cross compiler that generates ARM machine code, uses the embedded ABI, and can output bare-metal ELF binaries. The `none-eabi` part literally means “no OS / Embedded ABI”. When you set these compilers in the toolchain file, you’re defining the entire language runtime contract: calling convention, linking behavior, and where CMake looks for libraries.

The `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY` line matters because CMake likes to compile and run small test programs during configuration, which cannot work for a microcontroller target. This prevents CMake from trying to execute embedded code on your host during configuration.


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

This `CMakeLists.txt` is the build recipe for your firmware image. In embedded, the build system isn’t just responsible for turning `.c` into `.o` files — it’s responsible for producing a final artifact that matches a hardware memory map, includes the startup code, uses the correct CPU/FPU instruction set, and links in the right order. If you get build flags wrong, the firmware may still compile, but it can crash instantly at boot or behave subtly wrong.

The MCU flags are not optional details. They determine the exact machine code generated. The Cortex-M4F is a specific CPU core with an optional floating-point unit and a specific instruction set (Thumb). If you compile without `-mthumb`, `-mcpu=...`, or you mismatch `-mfloat-abi`, your code may build but won’t execute correctly, especially when function calls and floating point instructions come into play. In normal software dev, you rarely think about what CPU your code is running on. In embedded, that’s the whole game.

Linker options matter more than people expect. On a PC, the linker mostly figures itself out because the OS provides a loader and runtime. On bare metal, the linker script is the loader. It decides where code lives, where data lives, where the stack starts, and how the binary is arranged in memory. The map file (`firmware.map`) is your X-ray: when debugging early boot, the map file tells you exactly what went where.


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

One of the core embedded skills is being able to control hardware by writing to memory-mapped registers. Every peripheral in the MCU (GPIO, timers, UART, etc.) is controlled by writing to fixed addresses in memory. There’s no system call. There’s no driver layer unless you add one. When people say “bare metal”, they literally mean: your program writes 32-bit values to specific addresses and the hardware reacts.

Most vendor ecosystems hide this under CMSIS and HAL libraries. Those are useful, but they also hide what’s actually happening. This exercise intentionally defines only the minimum register structs needed, so you build the correct mental model: “A peripheral is just a region of memory. A register is just a variable at a fixed address. Writing bits configures the hardware.” Once you internalize that, every MCU becomes learnable.


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

A linker script is one of the big things that separates firmware from application programming. On your computer, you can pretend memory is infinite and flat. In firmware, memory is physical, limited, and split across different technologies. FLASH is non-volatile storage: it holds your code and constant data and survives power loss. RAM is volatile working memory: stack, heap, and variables. They are not interchangeable, and they are not at the same addresses.

The linker script describes the target’s memory layout and tells the linker where each section should live. This isn’t just about organization — it is required for boot. The CPU starts executing at a known address in flash. Your vector table must be at the beginning of flash. Your `.text` (code) must be in flash. Your `.data` is tricky: it must run in RAM (fast and writable), but it must be stored in flash (non-volatile). That’s why `.data` has both an LMA (load memory address) and VMA (virtual memory address). If you don’t understand that early, embedded will feel like black magic.


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

A linker script is one of the big things that separates firmware from application programming. On your computer, you can pretend memory is infinite and flat. In firmware, memory is physical, limited, and split across different technologies. FLASH is non-volatile storage: it holds your code and constant data and survives power loss. RAM is volatile working memory: stack, heap, and variables. They are not interchangeable, and they are not at the same addresses.

The linker script describes the target’s memory layout and tells the linker where each section should live. This isn’t just about organization — it is required for boot. The CPU starts executing at a known address in flash. Your vector table must be at the beginning of flash. Your `.text` (code) must be in flash. Your `.data` is tricky: it must run in RAM (fast and writable), but it must be stored in flash (non-volatile). That’s why `.data` has both an LMA (load memory address) and VMA (virtual memory address). If you don’t understand that early, embedded will feel like black magic.


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

A microcontroller’s clock setup controls everything: CPU speed, peripheral speed, timer rates, baud rates, and ultimately timing correctness. On reset, most MCUs start in a conservative default clock configuration (often an internal oscillator). That default is “safe and stable,” but it might not match what you want long term. In this exercise, we keep clocks simple on purpose so you can focus on fundamentals, but the pattern is important: system initialization is where you establish the platform assumptions your whole firmware will depend on.

The FPU enable is also a “firmware reality check” moment. On a Cortex-M4F, the floating-point unit exists but is often disabled at boot. If you compile code that uses floating point and the FPU isn’t enabled, the CPU will trap and you’ll hard fault. This is a classic embedded bug: perfectly valid C code that crashes instantly due to hardware configuration. You’re learning that firmware correctness is not only about syntax or algorithms — it’s about hardware state.


File: `exercise_1/src/system_stm32f4xx.c`

Keep clocks simple. A common minimal path is:
- leave the default HSI clock configuration
- set/maintain `SystemCoreClock` appropriately

Enable the FPU on Cortex-M4F if you are using it:
- CP10 and CP11 full access in SCB->CPACR

If you configure SysTick from `SystemCoreClock`, you must ensure it reflects reality.

## Part 6 — SysTick tick counter and GPIO blink

On embedded systems, you don’t get a free timer API from an OS. If you want millisecond timing, delays, or scheduling, you have to build it. SysTick is a standard Cortex-M timer specifically designed to provide a periodic interrupt “heartbeat”. It’s the simplest way to get a reliable timebase early in bringup. Many RTOS kernels use SysTick as their base scheduler tick, so learning it pays off.

Blinking an LED sounds trivial, but it’s actually the firmware equivalent of “Hello World” plus a bunch of real-world fundamentals: peripheral clock gating, pin multiplexing, register writes, timing, interrupts, and correctness under overflow. If your blink works and is stable, you’ve proven your toolchain, memory layout, startup code, and basic interrupt handling all function end-to-end.


File: `exercise_1/src/main.c`

### 6.1 SysTick tick

The SysTick interrupt handler is the smallest possible example of a hardware interrupt driving software state. The CPU is executing your loop, but at a fixed interval the hardware preempts your code, runs the ISR, then resumes. This is the foundational embedded concurrency model. There is no “thread” here — the interrupt is hardware-driven. If you do too much work in an ISR, you will destabilize your system. So we keep the ISR minimal: increment a counter and leave.

Configuring SysTick also forces you to think like firmware: timers don’t run in “milliseconds” by default. They run in clock cycles. You set `LOAD` to a value based on the CPU clock so the interrupt fires at the desired rate. That also means `SystemCoreClock` has to be correct. If it’s wrong, your timing will be wrong. This is why embedded devs obsess over clocks: everything is derived from them.

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

GPIO seems like the simplest peripheral, but it contains a lot of embedded lessons. First: clocks. Many peripherals are clock-gated, meaning they are physically disabled until you turn on their clock in RCC. If you forget this, your register writes will silently do nothing. Second: modes. Pins are multiplexed: the same pin can be GPIO, UART, SPI, etc. The MODER bits decide what the pin actually is.

The `BSRR` register is important because it teaches an atomic write pattern. On embedded systems, read-modify-write is not always safe, especially when interrupts are involved. If you read ODR, change one bit, and write it back, you can accidentally clobber changes made elsewhere (or in an ISR). BSRR avoids that by letting you set or reset bits with a single write, which is safer and also often faster. This is the kind of low-level detail that separates “it works” code from robust firmware code.


Enable the GPIO clock via RCC, configure PD12 as an output, then set/reset using BSRR:

```c
GPIOD->BSRR = (1U << 12);        // set PD12
GPIOD->BSRR = (1U << (12 + 16)); // reset PD12
```

Do not toggle by read-modify-write on ODR unless you know exactly why it is safe for your use case.

### 6.3 Wrap-safe scheduling

A lot of beginner embedded timing code looks correct and works for a while… then fails hours later in the field. The reason is timer overflow. Your tick counter is a fixed-width integer. It will wrap around back to zero eventually. If you compare time naïvely (like `if (now >= deadline)`), wrap breaks the logic and your scheduler can freeze or spam triggers.

The signed subtraction technique is a standard embedded pattern because it remains correct across wrap-around. When you compute `now - deadline` and interpret it as signed, the sign tells you whether the deadline is in the past or future even if the counter wrapped. This is a professional-grade habit worth learning early: firmware must be correct over long runtimes, not just “in the first 30 seconds of testing.”


Do not write timing code that fails on tick overflow. Use signed subtraction:

```c
if ((int32_t)(sys_tick - next_toggle) >= 0)
{
    next_toggle += period_ms;
    // toggle
}
```

## Part 7 — Verify memory placement

In firmware, debugging by “printf” is often not available at the beginning. Your best tools are the build artifacts: the ELF file, the map file, and objdump/readelf outputs. Verifying the section layout is how you confirm the firmware is shaped correctly for the MCU. If the vector table is not at the correct flash address, the CPU won’t boot. If `.data` load/run addresses are wrong, your globals will be garbage. If `.bss` isn’t cleared, your program starts in a random state.

This is why embedded engineers inspect binaries directly. On normal systems, the OS loader is doing this work for you. On bare metal, the binary layout is the contract between your code and the hardware. Learning to inspect it is learning to debug at the right level.


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

Renode is used here because it gives you a practical bringup loop without needing hardware on your desk. That matters for learning, and it matters for professional workflows too: you often want automated, deterministic, debuggable runs. Unlike some emulators, Renode is designed specifically for microcontrollers and includes peripheral models that make firmware development realistic.

The key point is: you’re not just “running code.” You’re validating that the MCU boots, the vector table and reset handler work, interrupts fire, and peripherals respond to register writes. Renode lets you observe those interactions directly. In early firmware bringup, observability is everything. If the system doesn’t boot, you need tools that let you see what it’s doing instead of guessing.


Use the provided script:

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
