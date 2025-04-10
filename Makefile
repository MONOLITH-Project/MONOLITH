# Toolchain
TOOLCHAIN_DIR := $(CURDIR)/toolchain
TOOLCHAIN_BIN := $(TOOLCHAIN_DIR)/bin
CROSS_PREFIX := x86_64-elf-
CC := $(TOOLCHAIN_BIN)/$(CROSS_PREFIX)gcc
LD := $(TOOLCHAIN_BIN)/$(CROSS_PREFIX)ld
AS := $(TOOLCHAIN_BIN)/$(CROSS_PREFIX)as

# Determine grub-mkrescue command to use
GRUB_MKRESCUE=$(shell command -v grub2-mkrescue)
ifeq (, $(GRUB_MKRESCUE))
	GRUB_MKRESCUE=$(shell command -v grub-mkrescue)
endif

# Directories
BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/isodir
OBJ_DIR := $(BUILD_DIR)/obj

# Output files
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
ISO_FILE := $(BUILD_DIR)/myos.iso

# Export variables for submakes
export BUILD_DIR TOOLCHAIN_BIN

# Compiler and linker flags
CFLAGS := -ffreestanding -mno-red-zone -g -Wall -Wextra -I./ -std=c99
LDFLAGS := -n -T boot/linker.ld -nostdlib

.PHONY: all clean toolchain iso run run-debug kernel boot test

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
	mkdir -p $(BUILD_DIR) $(BUILD_DIR)/obj

$(ISO_DIR)/boot/grub:
	mkdir -p $(ISO_DIR)/boot/grub

# Build kernel components
kernel: toolchain | $(BUILD_DIR)
	$(MAKE) -C kernel

# Build boot components
boot: toolchain | $(BUILD_DIR)
	$(MAKE) -C boot

# Run tests
test:
	$(MAKE) -C test/ all

# Generate test coverage report
coverage-report:
	$(MAKE) -C test/ coverage-report

# Link
$(KERNEL_BIN): kernel boot | toolchain
	$(LD) $(LDFLAGS) -o $@ $(shell find $(OBJ_DIR) -type f -name "*.o")

# Create ISO
$(ISO_FILE): $(KERNEL_BIN) | $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)

# Run in QEMU
run: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -serial stdio

run-headless: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -nographic

run-debug: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -serial stdio -s -S

run-debug-headless: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -nographic -s -S

# Clean build artifacts but keep toolchain
clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C boot clean
	rm -rf $(BUILD_DIR)

# Clean everything including toolchain
distclean: clean
	rm -rf $(TOOLCHAIN_DIR) .cache
