# KFS - Kernel From Scratch

## Description

This is a complete bootable kernel for the i386 architecture implementing advanced features including interrupts, memory management, paging, signals, and syscalls. The kernel has evolved through multiple iterations (KFS_1 through KFS_4) and now includes a comprehensive interrupt system with hardware interrupt handling.

## Current Version: KFS_4 - Interrupt System

The kernel now features a complete interrupt handling system with CPU exception handlers, hardware interrupts via the PIC (Programmable Interrupt Controller), signal-callback system, and syscall infrastructure.

## Project Structure

```
KFS1/
├── src/
│   ├── boot.s           # ASM boot code with multiboot header
│   ├── interrupt.s      # Low-level interrupt handlers (ISR/IRQ stubs)
│   ├── kernel.c         # Main kernel entry point
│   ├── idt.c            # Interrupt Descriptor Table implementation
│   ├── pic.c            # Programmable Interrupt Controller driver
│   ├── signal.c         # Signal system implementation
│   ├── syscall.c        # Syscall infrastructure
│   ├── gdt.c            # Global Descriptor Table (KFS_2)
│   ├── paging.c         # Memory paging system (KFS_3)
│   ├── kmalloc.c        # Physical memory allocator (KFS_3)
│   ├── vmalloc.c        # Virtual memory allocator (KFS_3)
│   ├── vga.c            # VGA text mode driver
│   ├── keyboard.c       # Interrupt-driven keyboard driver
│   ├── screen.c         # Multiple virtual screens
│   ├── shell.c          # Interactive shell
│   ├── printf.c         # Printf/printk implementation
│   ├── panic.c          # Kernel panic handler
│   ├── stack.c          # Stack utilities
│   └── string.c         # String functions
├── include/
│   ├── idt.h            # IDT definitions and interrupt frame structure
│   ├── pic.h            # PIC definitions
│   ├── signal.h         # Signal system interface
│   ├── syscall.h        # Syscall interface
│   ├── gdt.h            # GDT interface
│   ├── paging.h         # Paging interface
│   ├── kmalloc.h        # Physical memory allocator
│   ├── vmalloc.h        # Virtual memory allocator
│   ├── keyboard.h       # Keyboard driver interface
│   └── ...              # Other headers
├── linker.ld            # Linker script for i386
├── Makefile             # Build system
└── kfs1.iso             # Bootable ISO image (generated)
```

## Requirements

- **Architecture**: i386 (x86)
- **Assembler**: GNU as (for .s files)
- **Compiler**: GCC with multilib support (32-bit)
- **Bootloader**: GRUB
- **Tools**: grub-mkrescue (optional for ISO), qemu-system-i386 (for testing)

## Building

### Build kernel and ISO

```bash
make
```

This will compile the kernel and attempt to create a bootable ISO image `kfs1.iso` (requires grub-mkrescue).

### Build kernel only

```bash
make kernel
```

This will generate `kernel.bin` only.

### Clean build files

```bash
make clean
```

## Running

### With QEMU

```bash
qemu-system-i386 -kernel kernel.bin
```

Or with ISO:

```bash
qemu-system-i386 -cdrom kfs1.iso
```

### On real hardware

Write the ISO to a USB drive:

```bash
dd if=kfs1.iso of=/dev/sdX bs=4M
```

(Replace `/dev/sdX` with your USB device)

## Features

### KFS_4 - Interrupt System (Current) ✅

#### Mandatory Features
- ✅ **Interrupt Descriptor Table (IDT)**: Complete IDT with 256 entries
- ✅ **CPU Exception Handlers**: All 20 CPU exceptions (0x00-0x13) handled
  - Division by Zero, Debug, NMI, Breakpoint, Overflow
  - Bound Range Exceeded, Invalid Opcode, Device Not Available
  - Double Fault, Invalid TSS, Segment Not Present, Stack Fault
  - General Protection Fault, Page Fault, FPU Error, etc.
- ✅ **Hardware Interrupts (PIC)**: Programmable Interrupt Controller configured
  - PIC remapping to avoid conflicts with CPU exceptions
  - IRQ masking/unmasking support
  - EOI (End of Interrupt) handling
- ✅ **Signal-callback system**: Register and execute signal handlers
- ✅ **Signal scheduling**: Deferred signal execution
- ✅ **Register cleaning on panic**: Secure register clearing before halt
- ✅ **Stack saving on panic**: Stack state preserved and displayed
- ✅ **Keyboard interrupt handler**: IRQ1 for interrupt-driven keyboard input

#### Bonus Features
- ✅ **Syscall infrastructure**: INT 0x80 system call interface
- ✅ **Multiple keyboard layouts**: QWERTY, AZERTY, QWERTZ support
- ✅ **get_line() function**: Blocking input function for interactive shells
- ✅ **Full KFS_3 features**: GDT, Paging, Memory management included

### KFS_3 - Memory Management ✅

- ✅ **Global Descriptor Table (GDT)**: Proper segmentation setup
- ✅ **Paging**: Virtual memory with identity mapping and kernel heap
- ✅ **Physical Memory Allocator**: kmalloc/kfree for physical memory
- ✅ **Virtual Memory Allocator**: vmalloc/vfree for virtual memory
- ✅ **Memory information commands**: Display memory statistics

### KFS_2 - Stack Utilities ✅

- ✅ **Stack manipulation**: Stack pointer management
- ✅ **Stack testing**: Comprehensive stack tests

### KFS_1 - Base Kernel ✅

- ✅ **Bootable kernel**: GRUB multiboot specification
- ✅ **VGA text mode**: Full color support, cursor, scrolling
- ✅ **printf/printk**: Complete formatted output
- ✅ **Multiple virtual screens**: 4 independent screens (Alt+F1-F4)
- ✅ **Interactive shell**: Command-line interface

## Shell Commands

Type `help` in the shell for a complete list of commands:

- **help**: Display available commands
- **clear**: Clear the screen
- **stack [test|info]**: Stack operations and testing
- **reboot**: Reboot the system
- **mem [info]**: Display memory information
- **panic**: Trigger a kernel panic (demonstrates exception handling)
- **signal**: Test signal system
- **syscall**: Test syscall system
- **idt**: Display interrupt descriptor table information

## Keyboard Shortcuts

- **Alt+F1/F2/F3/F4**: Switch between virtual screens
- **Backspace**: Delete previous character
- **Enter**: Execute command or new line
- **Shift/Ctrl/Alt**: Modifier keys supported

## Technical Details

### Interrupt System

#### IDT (Interrupt Descriptor Table)
- 256 entries (0-255)
- Each entry: 8 bytes (offset, selector, type/attributes)
- Located in memory, loaded via `lidt` instruction

#### Exception Handlers
- CPU exceptions (0-19): Division errors, page faults, etc.
- Custom exception handler displays register dump and halts
- Page fault handler shows faulting address and error code details

#### Hardware Interrupts (IRQs)
- **PIC Configuration**: Master (IRQ0-7) and Slave (IRQ8-15) PICs
- **Remapping**: IRQ0-15 mapped to interrupts 32-47 (0x20-0x2F)
- **Interrupt Handlers**:
  - IRQ0 (0x20): Timer interrupt
  - IRQ1 (0x21): Keyboard interrupt
  - IRQ2-15: Available for other devices

#### Interrupt Frame Structure
The interrupt frame captures the complete processor state:
- General purpose registers (EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP)
- Segment registers (DS, ES, FS, GS)
- Interrupt number and error code
- Instruction pointer (EIP), code segment (CS), flags (EFLAGS)

### Signal System
- **Signal registration**: `signal_register(signal_num, handler)`
- **Signal raising**: `signal_raise(signal_num)`
- **Signal scheduling**: Signals executed at safe points
- **Signal handlers**: User-defined callback functions

### Syscall System
- **Interface**: INT 0x80 software interrupt
- **Registers**: EAX (syscall number), EBX, ECX, EDX (parameters)
- **Return**: EAX contains return value
- **Ring 3 accessible**: Can be called from user mode

### Memory Management

#### Paging
- **Page size**: 4 KB (4096 bytes)
- **Page directory**: 1024 entries
- **Page tables**: 1024 entries each
- **Identity mapping**: First 4 MB mapped 1:1
- **Kernel heap**: Dynamic allocation via vmalloc

#### Memory Allocators
- **kmalloc**: Physical memory allocation (4 KB blocks)
- **vmalloc**: Virtual memory allocation (4 KB pages)
- **Memory tracking**: Allocated blocks tracked for statistics

### VGA Text Mode
- **Address**: 0xB8000
- **Resolution**: 80x25 characters
- **Format**: 2 bytes per character (character + attribute)
- **Colors**: 16 foreground, 8 background colors
- **Cursor**: Hardware cursor via ports 0x3D4/0x3D5

### Keyboard
- **Interface**: PS/2 keyboard (interrupt-driven)
- **Port**: 0x60 (data), 0x64 (status)
- **Interrupt**: IRQ1 (vector 33/0x21)
- **Layouts**: QWERTY, AZERTY, QWERTZ
- **Buffer**: Circular buffer for input
- **Modifier keys**: Shift, Ctrl, Alt supported

## Compilation Flags

The kernel is compiled with strict flags to ensure no dependencies:

- `-m32`: Target i386 architecture
- `-fno-builtin`: No built-in functions
- `-fno-exceptions`: No C++ exceptions
- `-fno-stack-protector`: No stack protection
- `-nostdlib`: No standard library
- `-nodefaultlibs`: No default libraries
- `-Wall -Wextra`: All warnings enabled

## Memory Layout

```
0x00000000 - 0x000FFFFF : First 1 MB (BIOS, VGA, etc.)
0x00100000 - 0x???????? : Kernel code and data (loaded here by GRUB)
0xB8000000 - 0xBFFFFFFF : Kernel heap (vmalloc region)
0xC0000000 - 0xFFFFFFFF : Reserved
```

## Error Handling

### Kernel Panic
When a critical error occurs (e.g., unhandled exception):
1. Screen is cleared with red background
2. Exception name and error code displayed
3. Complete register dump shown
4. Page fault details (if applicable)
5. System halted safely

### Exception Information
- **Exception name**: Human-readable exception description
- **Error code**: Detailed error information
- **Register state**: All registers at time of exception
- **Page fault details**: Faulting address, access type, privilege level

## Code Quality

- **Modular architecture**: Clear separation of subsystems
- **Well-documented**: Comprehensive comments throughout
- **No standard library**: Completely standalone kernel
- **Bounds checking**: Input validation and safety checks
- **Error handling**: Proper error propagation and handling

## Testing

The kernel includes several test commands:
- `stack test`: Test stack operations
- `mem info`: Display memory statistics
- `panic`: Test exception handling
- `signal`: Test signal system
- `syscall`: Test syscall infrastructure
- `idt`: Display IDT information

## Development History

1. **KFS_1**: Base kernel with VGA, keyboard, printf
2. **KFS_2**: Stack utilities and manipulation
3. **KFS_3**: Memory management with paging
4. **KFS_4**: Complete interrupt system with signals and syscalls

## Author

Project created as part of the 42 School curriculum.

## License

This project is for educational purposes.
