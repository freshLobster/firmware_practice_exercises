#include <stdint.h>
#include "stm32f4xx_min.h"

uint32_t SystemCoreClock = 16000000; // Default HSI clock frequency

void SystemInit(void)
{
    volatile uint32_t *cpacr = (uint32_t *)0xE000ED88U; // Coprocessor Access Control Register
    *cpacr |= (0xF << 20); // Enable full access to CP10 and CP11 (FPU)

}

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = 16000000; // Default HSI clock frequency
}
