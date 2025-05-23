/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/memory.h>
#include <kernel/memory/pmm.h>
#include <kernel/serial.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <libs/limine/limine.h>
#include <stdint.h>

static uint8_t *_bitmap;
static uint8_t *_bitmap_end;
static size_t _bitmap_size;
static size_t _bitmap_page_count;
static size_t _physical_memory_size = 0;
static size_t _allocated_pages = 0;

__attribute__((used, section(".limine_requests"))) volatile struct limine_hhdm_request hhdmreq
    = {.id = LIMINE_HHDM_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests"))) volatile struct limine_memmap_request mmap_request
    = {.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

pmm_stats_t pmm_get_stats()
{
    return (pmm_stats_t) {
        .total_memory = _physical_memory_size,
        .total_pages = _bitmap_page_count,
        .used_pages = _allocated_pages,
        .free_pages = _bitmap_page_count - _allocated_pages,
    };
}

static void _mark_pages_used(size_t start_page, size_t number_of_pages)
{
    for (size_t j = start_page; j < start_page + number_of_pages; j++) {
        size_t byte_idx = j / 8;
        size_t bit_idx = j % 8;
        _bitmap[byte_idx] |= (1 << bit_idx);
    }
}

static void *_allocate_pages(size_t start_page, size_t number_of_pages)
{
    uint64_t base_addr = PHYSICAL_MEMORY_START + start_page * PAGE_SIZE;
    _mark_pages_used(start_page, number_of_pages);
    _allocated_pages += number_of_pages;
    debug_log_fmt("[*] pmm_alloc: Allocated %d pages at 0x%x\n", number_of_pages, base_addr);
    return (void *) base_addr;
}

static inline void *_process_byte(
    size_t byte, size_t *free_pages, size_t *start_page, size_t number_of_pages)
{
    size_t page_index = byte * 8;

    /* Skip scanning fully free bytes */
    if (_bitmap[byte] == 0x00) {
        if (*free_pages == 0) {
            *start_page = page_index;
        }
        *free_pages += 8;
        if (*free_pages >= number_of_pages) {
            return _allocate_pages(*start_page, number_of_pages);
        } else {
            return NULL;
        }
    }

    for (size_t bit = 0; bit < 8 && page_index + bit < _bitmap_page_count; bit++) {
        size_t i = page_index + bit;
        uint8_t bit_mask = 1 << bit;

        if (!(_bitmap[byte] & bit_mask)) {
            if (*free_pages == 0) {
                *start_page = i;
            }
            (*free_pages)++;

            if (*free_pages == number_of_pages) {
                return _allocate_pages(*start_page, number_of_pages);
            }
        } else {
            *free_pages = 0;
        }
    }

    return NULL;
}

static const char *_get_mmap_type(int t)
{
    switch (t) {
    case LIMINE_MEMMAP_USABLE:
        return "Usable";
    case LIMINE_MEMMAP_RESERVED:
        return "Reserved";
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return "ACPI Reclaimable";
    case LIMINE_MEMMAP_ACPI_NVS:
        return "ACPI NVS";
    case LIMINE_MEMMAP_BAD_MEMORY:
        return "Bad Memory";
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        return "Bootloader Reclaimable";
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:
        return "Kernel and Modules";
    case LIMINE_MEMMAP_FRAMEBUFFER:
        return "Framebuffer";
    default:
        return "Unknown";
    }
}

void pmm_init()
{
    debug_log("[*] Initializing PMM\n");

    struct limine_memmap_response *mmap_response = mmap_request.response;
    debug_log_fmt("[*] Number of memory map entries: %d\n", mmap_response->entry_count);

    /* Calculate the address of the end of kernel and physical memory start address and size */
    size_t kernel_end_addr = 0;
    struct limine_memmap_entry *biggest = NULL;
    for (size_t i = 0; i < mmap_response->entry_count; i++) {
        struct limine_memmap_entry *entry = mmap_response->entries[i];
        debug_log_fmt(
            "[*]\tMmap Entry %d (type: %s, base: 0x%x, length: %d bytes)\n",
            i,
            _get_mmap_type(entry->type),
            entry->base,
            entry->length);
        if (entry->type == LIMINE_MEMMAP_USABLE
            || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
            || entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            if (entry->base + entry->length > kernel_end_addr)
                kernel_end_addr = entry->base + entry->length;
        }
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            if (biggest == NULL || entry->length > biggest->length) {
                biggest = entry;
            }
            _physical_memory_size += entry->length;
        }
    }
    debug_log_fmt("[*] Found %d MB of physical memory\n", _physical_memory_size / 1048576);
    debug_log_fmt("[*] Physical memory start: 0x%x\n", PHYSICAL_MEMORY_START);

    _bitmap = (uint8_t *) biggest->base;
    _bitmap += hhdmreq.response->offset;
    _bitmap_page_count = _physical_memory_size / PAGE_SIZE;
    _bitmap_size = (_bitmap_page_count + 7) / 8; // Round up division
    _bitmap_end = _bitmap + _bitmap_size;

    debug_log_fmt("[*] End of kernel address: 0x%x\n", kernel_end_addr);
    debug_log_fmt("[*] Bitmap address range: 0x%x-0x%x\n", _bitmap, _bitmap_end);
    debug_log_fmt("[*] Bitmap page count: %d pages\n", _bitmap_page_count);
    debug_log_fmt("[*] Bitmap size: %d bytes\n", _bitmap_size);

    debug_log("[*] Initializing the bitmap...\n");
    for (size_t i = 0; i < _bitmap_size; i++)
        _bitmap[i] = 0;

    debug_log("[+] PMM initialized\n");
}

void *pmm_alloc(size_t pages)
{
    size_t current_free_pages = 0;
    size_t start_page = 0;
    void *result = NULL;

    for (size_t byte = 0; byte < _bitmap_size; byte++) {
        /* Skip fully used bytes */
        if (_bitmap[byte] == 0xFF) {
            current_free_pages = 0;
            continue;
        }

        result = _process_byte(byte, &current_free_pages, &start_page, pages);
        if (result) {
            return result + hhdmreq.response->offset;
        }
    }

    debug_log_fmt("[-] pmm_alloc failed: Could not find %d contiguous pages\n", pages);
    return NULL;
}

void pmm_free(void *ptr, size_t pages)
{
    if (ptr == NULL)
        return;

    uint64_t base_addr = (uint64_t) ptr;
    if (base_addr % PAGE_SIZE != 0) {
        debug_log_fmt("[!] pmm_free: Pointer 0x%lx is not page-aligned\n", base_addr);
        return;
    }
    size_t start_page = (base_addr - PHYSICAL_MEMORY_START) / PAGE_SIZE;
    if (start_page + pages > _bitmap_page_count) {
        debug_log_fmt("[!] pmm_free: Freeing %d pages at 0x%x exceeds bitmap\n", pages, base_addr);
        return;
    }

    /* Mark the pages as free */
    for (size_t i = start_page; i < start_page + pages; i++) {
        size_t byte_idx = i / 8;
        size_t bit_idx = i % 8;
        _bitmap[byte_idx] &= ~(1 << bit_idx);
    }

    _allocated_pages -= pages;
    debug_log_fmt("[*] pmm_free: Freed %d pages at 0x%x\n", pages, base_addr);
}
