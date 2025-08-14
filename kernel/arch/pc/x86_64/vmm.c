/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/asm.h>
#include <kernel/arch/pc/paging.h>
#include <kernel/debug.h>
#include <kernel/klibc/memory.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/terminal/kshell.h>
#include <kernel/video/panic.h>

__attribute__((used, section(".limine_requests"))) volatile struct limine_hhdm_request
    limine_hhdm_request
    = {.id = LIMINE_HHDM_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests"))) volatile struct limine_kernel_address_request
    limine_kernel_address_request
    = {.id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests"))) volatile struct limine_paging_mode_request
    limine_paging_request
    = {.id = LIMINE_PAGING_MODE_REQUEST, .mode = LIMINE_PAGING_MODE_X86_64_4LVL, .revision = 0};

static page_table_t *_pt_top_level;

extern void *_limine_requests_start;
extern void *_limine_requests_end;
extern void *_text_start;
extern void *_text_end;
extern void *_data_start;
extern void *_data_end;
extern void *_rodata_start;
extern void *_rodata_end;

void *vmm_get_hhdm_addr(void *phys_addr)
{
    return limine_hhdm_request.response->offset + phys_addr;
}

void *vmm_get_lhdm_addr(void *virt_addr)
{
    return virt_addr - limine_hhdm_request.response->offset;
}

static void _vmmap_command(int argc, char *argv[])
{
    if (argc != 4) {
        kprintf("\n[*] Usage: vmmap <virt> <phys> <flags>");
        return;
    }

    uintptr_t virt = atox(argv[1]);
    uintptr_t phys = atox(argv[2]);
    uint32_t flags = atox(argv[3]);

    vmm_map(virt, phys, flags, true);
}

static void _vminfo_command(int, char **)
{
    size_t present_pml5_entries = 0, present_pml4_entries = 0, present_pml3_entries = 0,
           present_pml2_entries = 0, present_pml1_entries = 0;

    /* Count present entries at each level.
     * I know this code is cursed, but it (probably) works.
     * TODO: refactor this shit.
     */
    for (int i = 0; i < 512; i++) {
        if (_pt_top_level->entries[i].flags.present) {
            present_pml4_entries++;
            page_table_t *pml3 = vmm_get_hhdm_addr(
                (void *) (_pt_top_level->entries[i].raw & ~0xFFF));
            for (int j = 0; j < 512; j++) {
                if (pml3->entries[j].flags.present) {
                    present_pml3_entries++;
                    page_table_t *pml2 = vmm_get_hhdm_addr((void *) (pml3->entries[j].raw & ~0xFFF));
                    for (int k = 0; k < 512; k++) {
                        if (pml2->entries[k].flags.present) {
                            present_pml2_entries++;
                            page_table_t *pml1 = vmm_get_hhdm_addr(
                                (void *) (pml2->entries[k].raw & ~0xFFF));
                            for (int l = 0; l < 512; l++) {
                                if (pml1->entries[l].flags.present) {
                                    present_pml1_entries++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    size_t total_mapped_pages = present_pml5_entries + present_pml4_entries + present_pml3_entries
                                + present_pml2_entries + present_pml1_entries;
    size_t total_mapped_memory_mb = total_mapped_pages * PAGE_SIZE / 1048576;

    kprintf("\n[*] Virtual Memory Information:\n");
    kprintf("[*] Page Table Top Level Address: 0x%x\n", _pt_top_level);
    kprintf("[*] Page Size: %d bytes\n", PAGE_SIZE);
    if (limine_paging_request.response->mode == LIMINE_PAGING_MODE_X86_64_5LVL)
        kprintf("[*] Present PML5 Entries: %d\n", present_pml5_entries);
    kprintf("[*] Present PML4 Entries: %d\n", present_pml4_entries);
    kprintf("[*] Present PML3 Entries: %d\n", present_pml3_entries);
    kprintf("[*] Present PML2 Entries: %d\n", present_pml2_entries);
    kprintf("[*] Present PML1 Entries: %d\n", present_pml1_entries);
    kprintf("[*] Total mapped pages: %d\n", total_mapped_pages);
    kprintf("[*] Total mapped memory: %d MB\n", total_mapped_memory_mb);
}

void vmm_init(struct limine_memmap_response *memmap_response)
{
    debug_log("[*] Initializing VMM...\n");
    debug_log("[*] Using Level-4 paging\n");

    _pt_top_level = pmm_alloc(1);
    if (_pt_top_level == NULL) {
        debug_log_fmt("[-] Failed to initialize the VMM");
        while (1)
            __asm__("hlt");
    }
    _pt_top_level = vmm_get_hhdm_addr(_pt_top_level);
    memset(_pt_top_level, 0, PAGE_SIZE);

    for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
        uint64_t type = memmap_response->entries[i]->type;
        if (type != LIMINE_MEMMAP_BAD_MEMORY) {
            vmm_map_range(
                (uintptr_t) vmm_get_hhdm_addr((void *) memmap_response->entries[i]->base),
                memmap_response->entries[i]->base,
                memmap_response->entries[i]->length,
                0x0000000000000003,
                false);
        }
    }

    uintptr_t kernel_paddr = limine_kernel_address_request.response->physical_base;
    uintptr_t kernel_vaddr = limine_kernel_address_request.response->virtual_base;

    uintptr_t limine_requests_start_vaddr = (uintptr_t) &_limine_requests_start;
    uintptr_t limine_requests_start_paddr = limine_requests_start_vaddr - kernel_vaddr
                                            + kernel_paddr;
    uintptr_t limine_requests_size = (uintptr_t) &_limine_requests_end
                                     - limine_requests_start_vaddr;
    uintptr_t text_start_vaddr = (uintptr_t) &_text_start;
    uintptr_t text_start_paddr = text_start_vaddr - kernel_vaddr + kernel_paddr;
    uintptr_t text_size = (uintptr_t) &_text_end - text_start_vaddr;
    uintptr_t rodata_start_vaddr = (uintptr_t) &_rodata_start;
    uintptr_t rodata_start_paddr = rodata_start_vaddr - kernel_vaddr + kernel_paddr;
    uintptr_t rodata_size = (uintptr_t) &_rodata_end - rodata_start_vaddr;
    uintptr_t data_start_vaddr = (uintptr_t) &_data_start;
    uintptr_t data_start_paddr = data_start_vaddr - kernel_vaddr + kernel_paddr;
    uintptr_t data_size = (uintptr_t) &_data_end - data_start_vaddr;

    vmm_map_range(
        limine_requests_start_vaddr, limine_requests_start_paddr, limine_requests_size, 0x03, false);
    vmm_map_range(text_start_vaddr, text_start_paddr, text_size, 0x03, false);
    vmm_map_range(rodata_start_vaddr, rodata_start_paddr, rodata_size, 0x03, false);
    vmm_map_range(data_start_vaddr, data_start_paddr, data_size, 0x03, false);

    asm_write_cr3((uintptr_t) vmm_get_lhdm_addr(_pt_top_level));

    debug_log_fmt("[*] The page table is located at 0x%x\n", _pt_top_level);
    kshell_register_command("vmmap", "Map virtual address to physical address", _vmmap_command);
    kshell_register_command("vminfo", "Display virtual memory statistics", _vminfo_command);

    debug_log("[+] Initialized VMM\n");
}

static inline void *_get_next_level(page_table_entry_t *entry)
{
    void *pt;
    if (!entry->flags.present) {
        pt = pmm_alloc(1);
        if (pt == NULL)
            return NULL;
        entry->raw = ((uint64_t) pt) | 0b111;
        return vmm_get_hhdm_addr(pt);
    }
    return vmm_get_hhdm_addr((void *) (entry->raw & ~0xFFF));
}

void vmm_map(uintptr_t virt, uintptr_t phys, size_t flags, bool flush)
{
    uint64_t pml4_index = PML4_GET_INDEX(virt);
    uint64_t pdpt_index = PML3_GET_INDEX(virt);
    uint64_t pdt_index = PML2_GET_INDEX(virt);
    uint64_t pt_index = PML1_GET_INDEX(virt);

    page_table_t *pdpt, *pdt, *pt;

    pdpt = _get_next_level(&_pt_top_level->entries[pml4_index]);
    if (pdpt == NULL)
        goto failure;

    pdt = _get_next_level(&pdpt->entries[pdpt_index]);
    if (pdt == NULL)
        goto failure;
    pt = _get_next_level(&pdt->entries[pdt_index]);
    if (pt == NULL)
        goto failure;

    pt->entries[pt_index].raw = (uintptr_t) phys | flags;
    if (flush) {
        debug_log_fmt("[*] Mapped phys 0x%x to virt 0x%x\n", phys, virt);
        asm_invlpg((void *) virt);
    }
    return;

failure:
    debug_log_fmt("[-] Failed to map virtual address 0x%x to physical address 0x%x\n", virt, phys);
}

void vmm_map_range(uintptr_t virt_addr, uintptr_t phys_addr, size_t size, size_t flags, bool flush)
{
    debug_log_fmt(
        "[*] Mapping 0x%x - 0x%x to 0x%x - 0x%x\n",
        phys_addr,
        phys_addr + size,
        virt_addr,
        virt_addr + size);
    size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; /* Round up */
    for (size_t i = 0; i < num_pages; i++)
        vmm_map(virt_addr + i * PAGE_SIZE, phys_addr + i * PAGE_SIZE, flags, false);
    if (flush)
        asm_write_cr3(asm_read_cr3());
}

void vmm_unmap(uintptr_t virt, bool flush)
{
    uint64_t pml4_index = PML4_GET_INDEX(virt);
    uint64_t pdpt_index = PML3_GET_INDEX(virt);
    uint64_t pdt_index = PML2_GET_INDEX(virt);
    uint64_t pt_index = PML1_GET_INDEX(virt);

    page_table_t *pdpt, *pdt, *pt;

    pdpt = _get_next_level(&_pt_top_level->entries[pml4_index]);
    if (pdpt == NULL)
        goto failure;

    pdt = _get_next_level(&pdpt->entries[pdpt_index]);
    if (pdt == NULL)
        goto failure;
    pt = _get_next_level(&pdt->entries[pdt_index]);
    if (pt == NULL)
        goto failure;

    pt->entries[pt_index].raw = 0;
    if (flush) {
        debug_log_fmt("[*] Unmapped virtual address 0x%x\n", virt);
        asm_invlpg((void *) virt);
    }

failure:
    debug_log_fmt("[-] Failed to unmap virtual address 0x%x\n", virt);
}

void vmm_unmap_range(uintptr_t virt_addr, size_t size, bool flush)
{
    debug_log_fmt("[*] Unmapping 0x%x - 0x%x\n", virt_addr, virt_addr + size);
    size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; /* Round up */
    for (size_t i = 0; i < num_pages; i++) {
        vmm_unmap(virt_addr + i * PAGE_SIZE, flush);
    }
}
