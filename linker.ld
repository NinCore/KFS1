/* Linker script for i386 kernel */

ENTRY(start)

SECTIONS
{
    /* Kernel starts at 1MB in physical memory */
    . = 0x00100000;

    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }
}
