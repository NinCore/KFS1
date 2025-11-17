# Makefile for KFS_1 kernel

# Compiler and assembler
CC = gcc
AS = nasm
LD = ld

# Flags as required by the subject
CFLAGS = -m32 -fno-builtin -fno-exceptions -fno-stack-protector -nostdlib -nodefaultlibs -Wall -Wextra -c
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Source files
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
ASM_SOURCES = $(wildcard $(SRC_DIR)/*.asm)

# Object files
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
ASM_OBJECTS = $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# Output kernel binary
KERNEL = kernel.bin

# ISO image for GRUB
ISO_DIR = isodir
ISO = kfs1.iso

.PHONY: all clean iso run kernel

all: iso

kernel: $(KERNEL)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@

# Assemble ASM source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link all object files
$(KERNEL): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# Create bootable ISO with GRUB
iso: $(KERNEL)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/$(KERNEL)
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "KFS_1" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '	multiboot /boot/$(KERNEL)' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '	boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

# Run with QEMU (fallback if KVM not available)
run: iso
	qemu-system-i386 -cdrom $(ISO)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(KERNEL) $(ISO_DIR) $(ISO)

# Additional helpful targets
help:
	@echo "Available targets:"
	@echo "  all    - Build the kernel binary (default)"
	@echo "  iso    - Create bootable ISO image"
	@echo "  run    - Run kernel in QEMU"
	@echo "  clean  - Remove all build artifacts"
	@echo "  help   - Show this help message"
