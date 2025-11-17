# KFS_1 - Kernel From Scratch

## Description

This is the first project in the Kernel From Scratch series. It implements a complete bootable kernel for the i386 architecture with VGA text mode, keyboard input, multiple virtual screens, and interactive features.

## Project Structure

```
KFS1/
├── src/
│   ├── boot.asm         # ASM boot code with multiboot header
│   ├── kernel.c         # Main kernel entry point with demo
│   ├── vga.c            # VGA text mode driver with cursor support
│   ├── keyboard.c       # PS/2 keyboard driver
│   ├── screen.c         # Multiple virtual screens management
│   ├── printf.c         # Printf/printk implementation
│   └── string.c         # Basic string functions
├── include/
│   ├── types.h          # Basic kernel types
│   ├── vga.h            # VGA driver interface
│   ├── keyboard.h       # Keyboard driver interface
│   ├── screen.h         # Screen management interface
│   ├── printf.h         # Printf functions
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

### Build everything (ISO included)

```bash
make
```

This will compile the kernel and create a bootable ISO image `kfs1.iso`.

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

### Mandatory (100% Complete)

- ✅ Bootable kernel via GRUB with multiboot specification
- ✅ ASM boot code handling multiboot header
- ✅ Basic kernel library with types and string functions
- ✅ VGA text mode driver for screen output
- ✅ Display "42" on screen (shown in green on main screen)
- ✅ Proper Makefile with correct compilation flags
- ✅ Custom linker script for i386
- ✅ Size under 10 MB (current: kernel ~18 KB, ISO ~5 MB)

### Bonus Features (All Implemented)

- ✅ **Scroll and cursor support**: Hardware cursor with blinking, automatic scrolling
- ✅ **Color support**: Full 16-color VGA palette with foreground/background
- ✅ **printf/printk helpers**: Complete printf implementation supporting:
  - `%s` - strings
  - `%c` - characters
  - `%d` / `%i` - signed integers
  - `%u` - unsigned integers
  - `%x` / `%X` - hexadecimal
  - `%%` - percent sign
- ✅ **Keyboard input handling**: Full PS/2 keyboard driver with:
  - US QWERTY layout
  - Shift, Ctrl, Alt modifier keys
  - Special keys (F-keys, Backspace, Enter, etc.)
  - Real-time input display
- ✅ **Multiple virtual screens**: 4 independent screens with:
  - Alt+F1 through Alt+F4 to switch
  - Each screen maintains its own content and cursor position
  - Smooth switching between screens

## Interactive Demo

When you boot the kernel, you'll see:

### Screen 0 (Main - Alt+F1)
- Welcome screen with feature list
- Display of "42" (mandatory requirement)
- Interactive command prompt
- Type to see keyboard input
- Color-coded text

### Screen 1 (Alt+F2)
- System information
- Technical details about the kernel
- Architecture and tools used

### Screen 2 (Alt+F3)
- Printf/printk test suite
- Demonstrates all format specifiers
- Shows various data types and formatting

### Screen 3 (Alt+F4)
- Color palette test
- Displays all 16 VGA colors with names
- Demonstrates color support

### Keyboard Commands

- **Type**: Characters appear on screen
- **Enter**: New line with prompt
- **Backspace**: Delete previous character
- **Alt+F1**: Switch to screen 0 (main)
- **Alt+F2**: Switch to screen 1 (info)
- **Alt+F3**: Switch to screen 2 (printf test)
- **Alt+F4**: Switch to screen 3 (color test)

## Compilation Flags

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
- **Colors**: 16 foreground colors, 8 background colors
- **Cursor**: Hardware cursor via VGA ports (0x3D4/0x3D5)

### Keyboard

- **Interface**: PS/2 keyboard controller
- **Ports**: 0x60 (data), 0x64 (status)
- **Scancodes**: Set 1 (XT)
- **Layout**: US QWERTY
- **Modifiers**: Shift, Ctrl, Alt

### Virtual Screens

- **Count**: 4 independent screens
- **Storage**: Each screen has a 4000-byte buffer (80x25x2)
- **Switching**: Saves current screen, restores target screen
- **Cursor**: Each screen maintains independent cursor position

### Multiboot

The kernel follows the Multiboot specification v1, allowing GRUB to load it. The multiboot header is located in the `.multiboot` section of the boot.asm file.

## Code Quality

- Clean, modular architecture
- Well-commented code
- Proper separation of concerns
- No memory leaks (no dynamic allocation used)
- No security vulnerabilities (bounds checking, input validation)

## Author

Project created as part of the 42 School curriculum.

## License

This project is for educational purposes.
