; boot.asm - Kernel entry point with multiboot header

; Multiboot constants
MULTIBOOT_MAGIC        equ 0x1BADB002
MULTIBOOT_ALIGN        equ 1 << 0
MULTIBOOT_MEMINFO      equ 1 << 1
MULTIBOOT_FLAGS        equ MULTIBOOT_ALIGN | MULTIBOOT_MEMINFO
MULTIBOOT_CHECKSUM     equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; Multiboot header
section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

; Stack setup
section .bss
align 16
stack_bottom:
    resb 16384  ; 16 KB stack
stack_top:

; Entry point
section .text
global start
extern kmain

start:
    ; Set up the stack
    mov esp, stack_top

    ; Call the kernel main function
    call kmain

    ; Hang if kmain returns
    cli
.hang:
    hlt
    jmp .hang
