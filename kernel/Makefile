PROJECT_ROOT := $(realpath ..)
OBJ_DIR := $(PROJECT_ROOT)/$(BUILD_DIR)/obj

# Toolchain
CROSS_PREFIX := x86_64-elf-
CC := $(TOOLCHAIN_BIN)/$(CROSS_PREFIX)gcc
AS := $(TOOLCHAIN_BIN)/$(CROSS_PREFIX)as

# Find all kernel source files
ASM_SRC := $(shell find . -name "*.asm" 2>/dev/null)
C_SRC := $(shell find . -name "*.c" 2>/dev/null)

# Generate object file paths
ASM_OBJ := $(patsubst %.asm,$(OBJ_DIR)/kernel/%.asm.o,$(ASM_SRC))
C_OBJ := $(patsubst %.c,$(OBJ_DIR)/kernel/%.c.o,$(C_SRC))
KERNEL_OBJ := $(ASM_OBJ) $(C_OBJ)

# Compiler flags
CFLAGS := -ffreestanding -mno-red-zone -g -Wall -Wextra -I../ -std=c99

.PHONY: all clean

all: $(KERNEL_OBJ)

# Create object output directories
define create_dir
	@mkdir -p $(dir $@)
endef

# Compile assembly
$(OBJ_DIR)/kernel/%.asm.o: %.asm
	$(create_dir)
	nasm -f elf64 $< -o $@

# Compile C files
$(OBJ_DIR)/kernel/%.c.o: %.c
	$(create_dir)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(ASM_OBJ) $(C_OBJ)
