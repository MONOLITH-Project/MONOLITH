/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "paging.h"
#include <kernel/klibc/memory.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/serial.h>
#include <stdint.h>

extern PDT_t *PML4T;

void vmm_init() {
    pmm_stats_t stats = pmm_get_stats();
    size_t pages_left = stats.total_pages - 512 * 2;
}

void vmm_map(void *virt, void *phys, size_t flags)
{
    debug_log_fmt("[*] Mapping phys 0x%x to virt 0x%x\n", phys, virt);
    uint64_t pml4_index = PML4_GET_INDEX(virt);
    uint64_t pdpt_index = PML3_GET_INDEX(virt);
    uint64_t pdt_index = PML2_GET_INDEX(virt);
    uint64_t pt_index = PML1_GET_INDEX(virt);

    PDT_t *pdpt, *pdt, *pt;

    if (!PML4T->entries[pml4_index].present) {
        pdpt = pmm_alloc(1);
        PML4T->entries[pml4_index].raw = (uint64_t) VIRT_TO_PHYS(pdpt) | flags;
    } else {
        pdpt = PHYS_TO_VIRT(PML4T->entries[pml4_index].raw & ~0xFFF);
    }

    if (!pdpt->entries[pdpt_index].present) {
        pdt = pmm_alloc(1);
        pdpt->entries[pdpt_index].raw = (uint64_t) VIRT_TO_PHYS(pdt) | flags;
    } else {
        pdt = PHYS_TO_VIRT(pdpt->entries[pdpt_index].raw & ~0xFFF);
    }

    if (!pdt->entries[pdt_index].present) {
        pt = pmm_alloc(1);
        pdt->entries[pdt_index].raw = (uint64_t) VIRT_TO_PHYS(pt) | flags;
    } else {
        pt = PHYS_TO_VIRT(pdt->entries[pdt_index].raw & ~0xFFF);
    }

    pt->entries[pt_index].raw = (uint64_t) phys | flags;

    /* Flush the TLB */
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_unmap(void *virt)
{
    debug_log_fmt("[*] Unmapping virt 0x%x\n", virt);

    uint64_t pml4_index = PML4_GET_INDEX(virt);
    uint64_t pdpt_index = PML3_GET_INDEX(virt);
    uint64_t pdt_index = PML2_GET_INDEX(virt);
    uint64_t pt_index = PML1_GET_INDEX(virt);

    PDT_t *pdpt, *pdt, *pt;

    if (!PML4T->entries[pml4_index].present)
        return;
    pdpt = PHYS_TO_VIRT(PML4T->entries[pml4_index].raw & ~0xFFF);

    if (!pdpt->entries[pdpt_index].present)
        return;
    pdt = PHYS_TO_VIRT(pdpt->entries[pdpt_index].raw & ~0xFFF);

    if (!pdt->entries[pdt_index].present)
        return;
    pt = PHYS_TO_VIRT(pdt->entries[pdt_index].raw & ~0xFFF);

    // Clear the page table entry
    pt->entries[pt_index].raw = 0;

    // Flush the TLB
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}
