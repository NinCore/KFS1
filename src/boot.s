/* boot.s - Kernel entry point with multiboot header (GNU as syntax) */

/* Multiboot constants */
.set MULTIBOOT_MAGIC,        0x1BADB002
.set MULTIBOOT_ALIGN,        1 << 0
.set MULTIBOOT_MEMINFO,      1 << 1
.set MULTIBOOT_FLAGS,        MULTIBOOT_ALIGN | MULTIBOOT_MEMINFO
.set MULTIBOOT_CHECKSUM,     -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

/* Multiboot header */
.section .multiboot
.align 4
    .long MULTIBOOT_MAGIC
    .long MULTIBOOT_FLAGS
    .long MULTIBOOT_CHECKSUM

/* Stack setup */
.section .bss
.align 16
.globl stack_bottom
.globl stack_top
stack_bottom:
    .skip 16384  /* 16 KB stack */
stack_top:

/* Entry point */
.section .text
.globl start
.globl gdt_flush
.extern kmain

start:
    /* Set up the stack */
    mov $stack_top, %esp

    /* Call the kernel main function */
    call kmain

    /* Hang if kmain returns */
    cli
hang:
    hlt
    jmp hang

/* gdt_flush - Load the GDT */
/* Takes one parameter: pointer to GDT pointer structure */
gdt_flush:
    mov 4(%esp), %eax       /* Get the pointer to the GDT pointer */
    lgdt (%eax)             /* Load the GDT */

    /* Reload segment registers */
    mov $0x10, %ax          /* 0x10 is the offset to kernel data segment */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* Far jump to reload CS (code segment) */
    ljmp $0x08, $flush      /* 0x08 is the offset to kernel code segment */

flush:
    ret
