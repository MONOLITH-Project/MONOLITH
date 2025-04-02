# Toolchain
TOOLCHAIN_DIR := $(CURDIR)/toolchain
TOOLCHAIN_BIN := $(TOOLCHAIN_DIR)/bin
CROSS_PREFIX := x86_64-elf-
CC := $(TOOLCHAIN_BIN)/$(CROSS_PREFIX)gcc
LD := $(TOOLCHAIN_BIN)/$(CROSS_PREFIX)ld
AS := $(TOOLCHAIN_BIN)/$(CROSS_PREFIX)as

# Directories
BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/isodir
OBJ_DIR := $(BUILD_DIR)/obj

# Output files
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
ISO_FILE := $(BUILD_DIR)/myos.iso

# Find all source files
BOOT_ASM_SRC := $(shell find boot -name "*.asm" 2>/dev/null)
KERNEL_ASM_SRC := $(shell find kernel -name "*.asm" 2>/dev/null)
ASM_SRC := $(BOOT_ASM_SRC) $(KERNEL_ASM_SRC)
C_SRC := $(shell find kernel -name "*.c" 2>/dev/null)

# Generate object file paths
ASM_OBJ := $(patsubst %,$(BUILD_DIR)/%.o,$(ASM_SRC))
C_OBJ := $(patsubst %,$(BUILD_DIR)/%.o,$(C_SRC))
ALL_OBJ := $(ASM_OBJ) $(C_OBJ)

# Compiler and linker flags
CFLAGS := -ffreestanding -mno-red-zone -g -Wall -Wextra -Iinclude -std=c99
LDFLAGS := -n -T boot/linker.ld -nostdlib

.PHONY: all clean toolchain iso run run-debug

all: $(ISO_FILE)

# Ensure toolchain is built
toolchain:
	@if [ ! -f "$(CC)" ]; then \
		echo "Building toolchain..."; \
		./scripts/build-toolchain.sh; \
	else \
		echo "Toolchain already exists at $(TOOLCHAIN_DIR)"; \
	fi

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ISO_DIR)/boot/grub:
	mkdir -p $(ISO_DIR)/boot/grub

# Create object output directories
define create_dir
	@mkdir -p $(dir $@)
endef

# Compile assembly
$(BUILD_DIR)/%.asm.o: %.asm | $(BUILD_DIR)
	$(create_dir)
	nasm -f elf64 $< -o $@

# Compile C files
$(BUILD_DIR)/%.c.o: %.c | $(BUILD_DIR) toolchain
	$(create_dir)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(KERNEL_BIN): $(ALL_OBJ) | toolchain
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(ALL_OBJ)

# Create ISO
$(ISO_FILE): $(KERNEL_BIN) | $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub2-mkrescue -o $@ $(ISO_DIR)

# Run in QEMU
run: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -serial stdio

run-debug: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -serial stdio -s -S

# Clean build artifacts but keep toolchain
clean:
	rm -rf $(BUILD_DIR)

# Clean everything including toolchain
distclean: clean
	rm -rf $(TOOLCHAIN_DIR) .cache
