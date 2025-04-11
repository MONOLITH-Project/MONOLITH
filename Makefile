# Toolchain
TOOLCHAIN_BASE_DIR := $(CURDIR)/toolchain
CPU_ARCH := $(patsubst pc/%,%,$(ARCH))
TOOLCHAIN_DIR := $(TOOLCHAIN_BASE_DIR)/$(CPU_ARCH)
TOOLCHAIN_BIN := $(TOOLCHAIN_DIR)/bin
# Architecture settings
ARCH ?= pc/x86_64
CROSS_PREFIX := x86_64-elf-
ifeq ($(ARCH),pc/i386)
	CROSS_PREFIX := i386-elf-
endif
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

# Arch detection and tracking
ARCH_FILE := $(BUILD_DIR)/.arch
# Function to check if architecture has changed and clean if needed
define check_arch_changed
	@mkdir -p $(BUILD_DIR)
	@if [ ! -f $(ARCH_FILE) ] || [ "$$(cat $(ARCH_FILE))" != "$(ARCH)" ]; then \
		echo "Architecture changed from $$(cat $(ARCH_FILE) 2>/dev/null || echo 'none') to $(ARCH). Cleaning build directory..."; \
		rm -rf $(BUILD_DIR)/obj; \
		mkdir -p $(BUILD_DIR); \
		echo "$(ARCH)" > $(ARCH_FILE); \
	fi
endef

# Output files
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
ISO_FILE := $(BUILD_DIR)/myos.iso

# Export variables for submakes
export BUILD_DIR TOOLCHAIN_BIN TOOLCHAIN_DIR TOOLCHAIN_BASE_DIR ARCH CROSS_PREFIX CPU_ARCH

# Compiler and linker flags
CFLAGS := -ffreestanding -g -Wall -Wextra -I./ -std=c99
CPU_ARCH := $(patsubst pc/%,%,$(ARCH))
ifeq ($(CPU_ARCH),x86_64)
	CFLAGS += -mno-red-zone
endif
LDFLAGS := -n -T boot/pc/$(CPU_ARCH)/linker.ld -nostdlib

.PHONY: all clean toolchain iso run run-debug kernel boot test

# Default target
all: | $(BUILD_DIR)
	$(call check_arch_changed)
	$(MAKE) kernel
	$(MAKE) boot
	$(MAKE) $(KERNEL_BIN)
	$(MAKE) $(ISO_FILE)

# Ensure toolchain is built
toolchain:
	@if [ ! -f "$(CC)" ] || [ ! -f "$(LD)" ]; then \
		echo "Building toolchain for $(ARCH) in $(TOOLCHAIN_DIR)..."; \
		ARCH="$(ARCH)" ./scripts/build-toolchain.sh; \
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
	cp boot/pc/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)

# Run in QEMU
run: all
ifeq ($(ARCH),pc/i386)
	qemu-system-i386 -cdrom $(ISO_FILE) -serial stdio
else
	qemu-system-x86_64 -cdrom $(ISO_FILE) -serial stdio
endif

run-headless: all
ifeq ($(ARCH),pc/i386)
	qemu-system-i386 -cdrom $(ISO_FILE) -nographic
else
	qemu-system-x86_64 -cdrom $(ISO_FILE) -nographic
endif

run-debug: all
ifeq ($(ARCH),pc/i386)
	qemu-system-i386 -cdrom $(ISO_FILE) -serial stdio -s -S
else
	qemu-system-x86_64 -cdrom $(ISO_FILE) -serial stdio -s -S
endif

run-debug-headless: all
ifeq ($(ARCH),pc/i386)
	qemu-system-i386 -cdrom $(ISO_FILE) -nographic -s -S
else
	qemu-system-x86_64 -cdrom $(ISO_FILE) -nographic -s -S
endif

# Clean build artifacts but keep toolchain
clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C boot clean
	rm -rf $(BUILD_DIR)

# Clean everything including toolchain
distclean: clean
	rm -rf $(TOOLCHAIN_BASE_DIR) .cache

# Force rebuild of toolchain for current architecture
rebuild-toolchain:
	@echo "Removing toolchain directory for $(CPU_ARCH) to ensure clean rebuild..."
	rm -rf $(TOOLCHAIN_DIR)
	@echo "Building toolchain for $(ARCH) in $(TOOLCHAIN_DIR)..."
	ARCH=$(ARCH) ./scripts/build-toolchain.sh

# Debugging target
debug-toolchain:
	@echo "Architecture (ARCH): $(ARCH)"
	@echo "CPU Architecture (CPU_ARCH): $(CPU_ARCH)"
	@echo "Expected compiler: $(CC)"
	@echo "Expected linker: $(LD)"
	@echo "Expected assembler: $(AS)"
	@echo "Toolchain directory contents:"
	@ls -la $(TOOLCHAIN_DIR)/bin/ || echo "Directory does not exist"
