# Toolchain
TOOLCHAIN_BASE_DIR := $(CURDIR)/toolchain
CPU_ARCH := $(patsubst pc/%,%,$(ARCH))
TOOLCHAIN_DIR := $(TOOLCHAIN_BASE_DIR)/$(CPU_ARCH)
TOOLCHAIN_BIN := $(TOOLCHAIN_DIR)/bin
# Architecture settings
ARCH ?= pc/x86_64
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
CFLAGS := -g -ffreestanding -Wall -Wextra -I./ -std=c99 -fno-stack-protector -fno-stack-check -fno-PIC
CPU_ARCH := $(patsubst pc/%,%,$(ARCH))
ifeq ($(CPU_ARCH),x86_64)
	CFLAGS += -mno-red-zone -mcmodel=kernel
endif
LDFLAGS += -T boot/pc/$(CPU_ARCH)/linker.ld -nostdlib -z max-page-size=0x1000 -static --build-id=none

FLANTERM_SOURCES := libs/flanterm/flanterm.c libs/flanterm/backends/fb.c
FLANTERM_OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(FLANTERM_SOURCES))

.PHONY: all clean toolchain iso run run-debug kernel test flanterm

# Default target
all: | $(BUILD_DIR)
	$(call check_arch_changed)
	$(MAKE) kernel
	$(MAKE) boot
	$(MAKE) flanterm
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

# Build FlanTerm library
flanterm: toolchain | $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)/libs/flanterm/backends
	$(CC) $(CFLAGS) -c libs/flanterm/flanterm.c -o $(OBJ_DIR)/libs/flanterm/flanterm.o
	$(CC) $(CFLAGS) -c libs/flanterm/backends/fb.c -o $(OBJ_DIR)/libs/flanterm/backends/fb.o

# Run tests
test:
	$(MAKE) -C test/ all

# Generate test coverage report
coverage-report:
	$(MAKE) -C test/ coverage-report

# Link
$(KERNEL_BIN): kernel flanterm | toolchain
	$(LD) $(LDFLAGS) -o $@ $(shell find $(OBJ_DIR) -type f -name "*.o")

# Create ISO
$(ISO_FILE): $(KERNEL_BIN)
	make -C libs/limine
	mkdir -p build/iso/boot/limine
	cp -v $(KERNEL_BIN) build/iso/boot
	cp -v boot/pc/limine.conf boot/pc/wallpaper.png libs/limine/limine-bios.sys \
	    libs/limine/limine-bios-cd.bin libs/limine/limine-uefi-cd.bin \
		build/iso/boot/limine
	mkdir -p build/iso/EFI/BOOT
	cp -v libs/limine/BOOTX64.EFI build/iso/EFI/BOOT
	cp -v libs/limine/BOOTIA32.EFI build/iso/EFI/BOOT
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
	    -no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		--efi-boot-part --efi-boot-image --protective-msdos-label \
		build/iso/ -o $(ISO_FILE)

# Run in QEMU
run: all
	qemu-system-x86_64 -cdrom $(ISO_FILE) -serial stdio

run-headless: all
	qemu-system-x86_64 -cdrom $(ISO_FILE) -nographic

run-debug: all
	qemu-system-x86_64 -cdrom $(ISO_FILE) -serial stdio -s -S

run-debug-headless: all
	qemu-system-x86_64 -cdrom $(ISO_FILE) -nographic -s -S

# Clean build artifacts but keep toolchain
clean:
	$(MAKE) -C kernel clean
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
