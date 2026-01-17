#pragma once
#include <stdint.h>

//Reset and Clock Control
typedef struct
{
  volatile uint32_t CR;
  volatile uint32_t PLLCFGR;
  volatile uint32_t CFGR;
  volatile uint32_t CIR;
  volatile uint32_t AHB1RSTR;
  volatile uint32_t AHB2RSTR;
  volatile uint32_t AHB3RSTR;
  uint32_t      RESERVED0;
  volatile uint32_t APB1RSTR;
  volatile uint32_t APB2RSTR;
  uint32_t      RESERVED1[2];
  volatile uint32_t AHB1ENR;
} RCC_TypeDef;

/* Base addresses (from RM0090) */
#define PERIPH_BASE 0x40000000UL
#define RCC_BASE    (PERIPH_BASE + 0x00023800UL)

/* Typed pointer to RCC */
#define RCC ((RCC_TypeDef *)RCC_BASE)

/* Bit to enable GPIOD clock (LED port on STM32F4-Discovery) */
#define RCC_AHB1ENR_GPIODEN (1U << 3)

// GPIO register layout (per pin: 16 pins on port D)
typedef struct
{
    volatile uint32_t MODER;    // Mode (2 bits per pin)
    volatile uint32_t OTYPER;   // Output type (1 bit per pin)
    volatile uint32_t OSPEEDR;  // Output speed (2 bits per pin)
    volatile uint32_t PUPDR;    // Pull-up/pull-down (2 bits per pin)
    volatile uint32_t IDR;      // Input data (1 bit per pin)
    volatile uint32_t ODR;      // Output data (1 bit per pin)
    volatile uint32_t BSRR;     // Bit set/reset (lower 16 set, upper 16 reset)
    volatile uint32_t LCKR;     // Configuration lock
    volatile uint32_t AFR[2];   // Alternate function low/high (pins 0-7, 8-15)
} GPIO_TypeDef;

// GPIO port D base and pointer
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define GPIOD_BASE      (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOD           ((GPIO_TypeDef *)GPIOD_BASE)

//core system blocks

//System Control Block (SCB) struct
//cortex-m4 generic user manual p. 80
typedef struct
{
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint8_t  SHP[12];
    volatile uint32_t SHCSR;
} SCB_Type;

//SysTick struct
//cortex-m4 generic user manual p. 102
typedef struct
{
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_Type;

//Base addresses
//cortex-m4 generic user manual p. 15
#define SCS_BASE            (0xE000E000UL)                            /*!< System Control Space Base Address */
#define SCB_BASE            (SCS_BASE +  0x0D00UL)                    /*!< System Control Block Base Address */
#define SysTick_BASE        (SCS_BASE +  0x0010UL)                    /*!< SysTick Base Address */
//Typed pointers to core system blocks
#define SysTick             ((SysTick_Type *)SysTick_BASE)            /*!< SysTick configuration struct */
#define SCB                 ((SCB_Type *)SCB_BASE)                    /*!< SCB configuration struct */
//SysTick bit masks
#define SYSTICK_CTRL_ENABLE     (1U << 0)                              /*!< SysTick Counter Enable */
#define SYSTICK_CTRL_TICKINT    (1U << 1)                              /*!< SysTick Interrupt Enable */
#define SYSTICK_CTRL_CLKSOURCE  (1U << 2)                              /*!< SysTick Clock Source */

void SystemCoreClockUpdate(void); // Prototype for system clock update function