PROJECT_ROOT := $(realpath ..)
OBJ_DIR := $(PROJECT_ROOT)/$(BUILD_DIR)/obj

# Find all boot source files
ASM_SRC := $(shell find . -name "*.asm" 2>/dev/null)

# Generate object file paths
ASM_OBJ := $(patsubst %.asm,$(OBJ_DIR)/boot/%.o,$(ASM_SRC))
BOOT_OBJ := $(ASM_OBJ)

.PHONY: all clean

all: $(BOOT_OBJ)

# Create object output directories
define create_dir
	@mkdir -p $(dir $@)
endef

# Compile assembly
$(OBJ_DIR)/boot/%.o: %.asm
	$(create_dir)
	nasm -f elf64 $< -o $@

clean:
	rm -rf $(ASM_OBJ)
