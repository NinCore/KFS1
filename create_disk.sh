#!/bin/bash
# Script to create an EXT2 filesystem image for KFS-6 testing

echo "Creating EXT2 disk image for KFS-6..."

# Create a 10MB disk image
dd if=/dev/zero of=disk.img bs=1M count=10 2>/dev/null

# Format it as EXT2
mkfs.ext2 -F disk.img >/dev/null 2>&1

# Mount it temporarily
mkdir -p /tmp/kfs_mount
sudo mount -o loop disk.img /tmp/kfs_mount 2>/dev/null

# Create directory structure
sudo mkdir -p /tmp/kfs_mount/home
sudo mkdir -p /tmp/kfs_mount/dev
sudo mkdir -p /tmp/kfs_mount/tmp
sudo mkdir -p /tmp/kfs_mount/etc

# Create some test files
echo "Welcome to KFS-6 Filesystem!" | sudo tee /tmp/kfs_mount/welcome.txt >/dev/null
echo "This is a test file on EXT2 disk" | sudo tee /tmp/kfs_mount/test.txt >/dev/null
echo "Hello from /home directory!" | sudo tee /tmp/kfs_mount/home/hello.txt >/dev/null

# Create a larger test file
echo "KFS-6 - Filesystem Implementation" | sudo tee /tmp/kfs_mount/readme.txt >/dev/null
echo "=================================" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "This disk image contains:" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "- EXT2 filesystem" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "- Multiple directories (/home, /dev, /tmp, /etc)" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "- Test files to read with cat command" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "Try these commands:" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "  ls /" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "  cat /readme.txt" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "  cd /home" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "  pwd" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null
echo "  ls" | sudo tee -a /tmp/kfs_mount/readme.txt >/dev/null

# Unmount
sudo umount /tmp/kfs_mount 2>/dev/null
rmdir /tmp/kfs_mount 2>/dev/null

echo "✓ EXT2 disk image created: disk.img (10 MB)"
echo "✓ Contains: /home, /dev, /tmp, /etc directories"
echo "✓ Test files: welcome.txt, test.txt, readme.txt, /home/hello.txt"
echo ""
echo "To use with QEMU, add to your run command:"
echo "  -drive file=disk.img,format=raw,if=ide"
echo ""
echo "Or use the new 'make run-disk' target!"
