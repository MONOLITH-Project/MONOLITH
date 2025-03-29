# Directories
BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/isodir

# Output files
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
ISO_FILE := $(BUILD_DIR)/myos.iso

# Source files
ASM_SRC := boot/boot.asm
CFILES_SRC := kernel/kernel.c

# Object files
ASM_OBJ := $(BUILD_DIR)/boot.o
CFILES_OBJ := $(BUILD_DIR)/kernel.o

.PHONY: all clean iso

all: $(ISO_FILE)

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ISO_DIR)/boot/grub:
	mkdir -p $(ISO_DIR)/boot/grub

# Compile assembly
$(ASM_OBJ): $(ASM_SRC) | $(BUILD_DIR)
	nasm -f elf64 $< -o $@

# Compile C files
$(CFILES_OBJ): $(CFILES_SRC) | $(BUILD_DIR)
	gcc -c $< -o $@ -ffreestanding -g -Wall -Wextra

# Link
$(KERNEL_BIN): $(ASM_OBJ) $(CFILES_OBJ)
	ld -n -o $@ -T boot/linker.ld $^

# Create ISO
$(ISO_FILE): $(KERNEL_BIN) | $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub2-mkrescue -o $@ $(ISO_DIR)

# Run in QEMU
run: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE)

run-debug: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -s -S

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
