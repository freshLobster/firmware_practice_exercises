.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.extern SystemInit
.extern main

.section .isr_vector, "a", %progbits

.global Reset_Handler

g_pfnVectors:
    .word   _estack                     // Initial Stack Pointer
    .word   Reset_Handler               // Reset Handler
    .word   NMI_Handler                 // NMI Handler
    .word   HardFault_Handler           // Hard Fault Handler
    .word   MemManage_Handler           // MPU Fault Handler
    .word   BusFault_Handler            // Bus Fault Handler
    .word   UsageFault_Handler          // Usage Fault Handler
    .word   0                           // Reserved
    .word   0                           // Reserved
    .word   0                           // Reserved
    .word   0                           // Reserved
    .word   SVC_Handler                 // SVCall Handler
    .word   DebugMon_Handler            // Debug Monitor Handler
    .word   0                           // Reserved
    .word   PendSV_Handler              // PendSV Handler
    .word   SysTick_Handler             // SysTick Handler

    .text

    .thumb_func
Reset_Handler:                          // Reset Handler
    ldr   r0, =_estack            // Load address of stack pointer
    mov   sp, r0                  // Set stack pointer
    bl    SystemInit              // Call system initialization
    ldr   r0, =_sdata             // Load start address of .data
    ldr   r1, =_edata             // Load end address of .data
    ldr   r2, =_sidata            // Load start address of .idata
1:  
    cmp   r0, r1                  // Compare current address with end address
    ittt lt                       // If less than, then
    ldrlt r3, [r2], #4            // Load word from .idata
    strlt r3, [r0], #4            // Store word to .data
    blt   1b                      // Repeat until all data is copied

    ldr   r0, =_sbss              // Load start address of .bss
    ldr   r1, =_ebss              // Load end address of .bss
    movs r2, #0                   // Zero value

2:  
    cmp   r0, r1                  // Compare current address with end address
    it   lt                       // If less than, then
    strlt r2, [r0], #4            // Store zero to .bss
    blt   2b                      // Repeat until all .bss is cleared
    bl    main                    // Call main function

3:  
    b     3b                      // Infinite loop if main returns

.weak NMI_Handler                 //can be overridden by user-defined handlers
.weak HardFault_Handler
.weak MemManage_Handler
.weak BusFault_Handler
.weak UsageFault_Handler
.weak SVC_Handler
.weak DebugMon_Handler
.weak PendSV_Handler
.weak SysTick_Handler

    .thumb_func
Default_Handler:
    b .


    .thumb_func
NMI_Handler:                            
    b Default_Handler
    .thumb_func
HardFault_Handler:
    b Default_Handler
    .thumb_func
MemManage_Handler:
    b Default_Handler
    .thumb_func
BusFault_Handler:
    b Default_Handler
    .thumb_func
UsageFault_Handler:
    b Default_Handler
    .thumb_func
SVC_Handler:
    b Default_Handler
    .thumb_func
DebugMon_Handler:
    b Default_Handler
    .thumb_func
PendSV_Handler:
    b Default_Handler
    .thumb_func
SysTick_Handler:
    b Default_Handler
