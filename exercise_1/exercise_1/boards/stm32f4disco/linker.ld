/* STM32F407VG: 1MB FLASH @0x08000000, 128KB SRAM @0x20000000 */
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } >FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.init)
        *(.fini)
        KEEP(*(.init))
        KEEP(*(.fini))
    } >FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } >FLASH

    . = ALIGN(4);
    _sidata = .;
    .data : AT(_sidata)
    {
        _sdata = .;
        *(.data*)
        _edata = .;
    } >RAM

    .bss (NOLOAD) :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } >RAM

    .heap (NOLOAD) :
    {
        . = ALIGN(8);
    __HeapBase = .;
    . = . + 0x8000; /* 32 KB heap */
        __HeapLimit = .;
    } >RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        __StackLimit = .;
        . = ORIGIN(RAM) + LENGTH(RAM);
        __StackTop = .;
    } >RAM

    /DISCARD/ :
    {
        *(.comment)
    }

}
