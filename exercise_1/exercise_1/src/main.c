#include <stdint.h>
#include "stm32f4xx_min.h"

#define LED_PIN 12

static volatile uint32_t sys_tick = 0; // SysTick counter
static uint8_t led_state = 0;          // LED state: 0 = OFF, 1 = ON
extern uint32_t SystemCoreClock; // System core clock frequency (from system_stm32f4xx.c)

void SysTick_Handler(void)
{
    ++sys_tick; // Increment SysTick counter
}

void GPIOD_Init(uint8_t pin)
{
    // Enable GPIOD clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    GPIOD->MODER &= ~(0x03U << (pin * 2));   // Clear mode bits
    GPIOD->MODER |= (0x01U << (pin * 2));    // set pin mode to output
    GPIOD->OTYPER &= ~(0x01U << pin);        // Push-pull output mode
    GPIOD->OSPEEDR &= ~(0x03U << (pin * 2)); // Clear speed bits
    GPIOD->OSPEEDR |= (0x02U << (pin * 2));  // medium speed
    GPIOD->PUPDR &= ~(0x03U << (pin * 2));   // no pull-up/down
}

void SysTick_Init(void)
{
    SCB->SHP[11] = 0xF0;                                                                 // Set SysTick interrupt priority to lowest (15)
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;                                      // Set reload register for 1ms interrupts
    SysTick->VAL = 0U;                                                                   // Load the SysTick Counter Value
    SysTick->CTRL = SYSTICK_CTRL_ENABLE | SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT; // Enable SysTick IRQ and SysTick Timer
}

static inline void gpio_set_pin(uint8_t pin)
{
    GPIOD->BSRR = (1U << pin); // Set pin
    led_state = 1;             // LED is ON
}

static inline void gpio_clear_pin(uint8_t pin)
{
    GPIOD->BSRR = (1U << (pin + 16)); // Reset pin
    led_state = 0;                    // LED is OFF
}

int main(void)
{
    SystemCoreClockUpdate(); // Update SystemCoreClock variable
    GPIOD_Init(LED_PIN);     // Initialize GPIOD pin for LED
    SysTick_Init();          // Initialize SysTick timer

    const uint32_t period = 500;              // Blink period in ms
    uint32_t next_toggle = sys_tick + period; // Next toggle time
    for (;;)                                  // Main loop
    {
        if ((int32_t)(sys_tick - next_toggle) >= 0) // Check if it's time to toggle LED state
        {
            led_state ? gpio_clear_pin(LED_PIN) : gpio_set_pin(LED_PIN); // Toggle LED state
            next_toggle += period;                                       // Update next toggle time
        }
    }
}