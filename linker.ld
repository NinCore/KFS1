/* Linker script for i386 kernel */

ENTRY(start)

SECTIONS
{
    /* GDT must be at address 0x00000800 as per KFS_2 subject */
    . = 0x00000800;

    .gdt : {
        *(.gdt)
    }

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
