/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libs/limine/limine.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Initialize the virtual memory manager.
 */
void vmm_init(struct limine_memmap_response *);

/*
 * Map a single page from a physical to a virtual address.
 */
void vmm_map(uintptr_t virt_addr, uintptr_t phys_addr, size_t flags, bool flush);

/*
 * Map a specified number of pages from a physical to a virtual address.
 */
void vmm_map_range(uintptr_t virt_addr, uintptr_t phys_addr, size_t size, size_t flags, bool flush);

/*
 * Unmap a single page from a virtual address.
 */
void vmm_unmap(uintptr_t virt_addr, bool flush);

/*
 * Unmap a specified number of pages from a virtual address.
 */
void vmm_unmap_range(uintptr_t virt_addr, size_t size, bool flush);

/*
 * Convert a physical address to a high-half direct mapped address.
 */
void *vmm_get_hhdm_addr(void *phys_addr);

/*
 * Convert a higher-half direct mapped virtual address to the physical address.
 */
void *vmm_get_lhdm_addr(void *virt_addr);
