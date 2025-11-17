# KFS_1 - Kernel From Scratch

## Description

This is the first project in the Kernel From Scratch series. It implements a minimal bootable kernel for the i386 architecture that displays "42" on the screen using VGA text mode.

## Project Structure

```
KFS1/
├── src/
│   ├── boot.asm         # ASM boot code with multiboot header
│   ├── kernel.c         # Main kernel entry point
│   ├── vga.c            # VGA text mode driver
│   └── string.c         # Basic string functions (strlen, strcmp, etc.)
├── include/
│   ├── types.h          # Basic kernel types
│   ├── vga.h            # VGA driver interface
│   └── string.h         # String functions interface
├── linker.ld            # Linker script for i386
├── Makefile             # Build system
└── kfs1.iso             # Bootable ISO image (generated)
```

## Requirements

- **Architecture**: i386 (x86)
- **Assembler**: NASM
- **Compiler**: GCC with multilib support
- **Bootloader**: GRUB
- **Tools**: grub-mkrescue, xorriso, qemu-system-i386 (for testing)

## Building

### Compile the kernel

```bash
make
```

This will generate `kernel.bin`.

### Create bootable ISO

```bash
make iso
```

This will create `kfs1.iso` with GRUB.

### Clean build files

```bash
make clean
```

## Running

### With QEMU

```bash
make run
```

Or manually:

```bash
qemu-system-i386 -cdrom kfs1.iso
```

### With KVM

```bash
kvm -cdrom kfs1.iso
```

### On real hardware

Write the ISO to a USB drive:

```bash
dd if=kfs1.iso of=/dev/sdX bs=4M
```

(Replace `/dev/sdX` with your USB device)

## Features

### Mandatory

- ✅ Bootable kernel via GRUB
- ✅ ASM boot code with multiboot header
- ✅ Basic kernel library with types and functions
- ✅ VGA text mode driver
- ✅ Display "42" on screen
- ✅ Proper Makefile with correct compilation flags
- ✅ Custom linker script
- ✅ Size under 10 MB (current: ~4.9 MB)

### Compilation Flags

The kernel is compiled with the following flags to ensure no dependencies on host libraries:

- `-m32`: Target i386 architecture
- `-fno-builtin`: No built-in functions
- `-fno-exceptions`: No C++ exceptions
- `-fno-stack-protector`: No stack protection
- `-nostdlib`: No standard library
- `-nodefaultlibs`: No default libraries
- `-Wall -Wextra`: All warnings enabled

## Technical Details

### Memory Layout

The kernel is loaded at `0x00100000` (1 MB) by GRUB, as specified in `linker.ld`.

### VGA Text Mode

- **Address**: 0xB8000
- **Resolution**: 80x25 characters
- **Format**: Each character is 2 bytes (character + color attribute)

### Multiboot

The kernel follows the Multiboot specification, allowing GRUB to load it. The multiboot header is located in the `.multiboot` section.

## Author

Project created as part of the 42 School curriculum.

## License

This project is for educational purposes.
