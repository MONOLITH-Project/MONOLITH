PROJECT_ROOT := $(realpath ../..)
OBJ_DIR := $(PROJECT_ROOT)/$(BUILD_DIR)/obj

# CPU_ARCH is now imported from parent Makefile

# Find all boot source files for the target architecture
ASM_SRC := $(shell find $(CPU_ARCH) -name "*.asm" 2>/dev/null)

# Generate object file paths
ASM_OBJ := $(patsubst %.asm,$(OBJ_DIR)/boot/pc/%.o,$(ASM_SRC))
BOOT_OBJ := $(ASM_OBJ)

.PHONY: all clean

all: $(BOOT_OBJ)
	@echo "PC boot files for $(CPU_ARCH) built successfully"

# Create object output directories
define create_dir
	@mkdir -p $(dir $@)
endef

# Compile assembly
ifeq ($(CPU_ARCH),x86_64)
$(OBJ_DIR)/boot/pc/%.o: %.asm
	$(create_dir)
	nasm -f elf64 $< -o $@
else ifeq ($(CPU_ARCH),i386)
$(OBJ_DIR)/boot/pc/%.o: %.asm
	$(create_dir)
	nasm -f elf32 $< -o $@
else
$(error Unsupported CPU architecture: $(CPU_ARCH))
endif

clean:
	rm -rf $(ASM_OBJ)
