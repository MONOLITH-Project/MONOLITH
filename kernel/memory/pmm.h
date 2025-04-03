/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include "../multiboot2.h"
#include <stddef.h>

#define PAGE_SIZE 4096
#define PHYSICAL_MEMORY_START 0x100000

/*
 * Find the memory map tag in the Multiboot info structure.
 * Returns a pointer to the memory map tag if found, otherwise NULL.
 * https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Boot-information-format
 */
struct multiboot_tag_mmap *find_mmap_tag(struct multiboot_tag *);

/*
 * Initialize the Physical Memory Manager.
 */
void init_pmm(struct multiboot_tag_mmap *);

/*
 * Allocate a free page from physical memory.
 * Returns a pointer to the allocated page if successful, otherwise NULL.
 */
void *pmm_alloc(size_t);

/*
 * Free a page of physical memory.
 * Does nothing if the parameter is NULL.
 */
void pmm_free(void *);
