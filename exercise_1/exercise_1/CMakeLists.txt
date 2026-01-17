# CMAKE project list for exercise_1

# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.24)

# Use a custom toolchain file for ARM Cortex-M development
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi.cmake" CACHE FILEPATH "" FORCE)

# Define the project name and supported languages
project(stm32f4_freertos C ASM)

# Set the compiler flags for the project
set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

# Enable verbose output for compilation commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the target type for try_compile to STATIC_LIBRARY
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set the target board for the project
set(BOARD stm32f4disco)

# Add the executable target for the firmware
add_executable(firmware.elf
    src/startup_stm32f407xx.s
    src/system_stm32f4xx.c
    src/main.c
)

# Include directories for the target
target_include_directories(firmware.elf PRIVATE
    include
    boards/${BOARD}
)

# Target compile options for the firmware
target_compile_options(firmware.elf PRIVATE
    ${MCU_FLAGS}
    -Wall
    -Wextra
    -Werror
    -ffunction-sections
    -fdata-sections
)

# Linker options for the firmware
target_link_options(firmware.elf PRIVATE
    ${MCU_FLAGS}
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map
    -T${CMAKE_SOURCE_DIR}/boards/${BOARD}/linker.ld
)