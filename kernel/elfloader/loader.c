/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/asm.h>
#include <kernel/debug.h>
#include <kernel/elfloader/elf.h>
#include <kernel/elfloader/loader.h>
#include <kernel/elfloader/usermode.h>
#include <kernel/fs/vfs.h>
#include <kernel/klibc/memory.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <stdint.h>

typedef int (*func_t)(int argc, char **argv);

int load_elf(file_t *file)
{
    debug_log("[*] Loading ELF file...\n");
    elf64_header_t header;
    if (parse_elf_header(file, &header) < 0) {
        debug_log("[-] ELF parsing error\n");
        return -1;
    }
    debug_log("[*] Loaded ELF file\n");

    if (header.header.format != ELF_FORMAT_64BIT) {
        debug_log("[-] ELF is not 64-bit\n");
        return -1;
    } else if (header.header.isa != ELF_ISA_X86_64) {
        debug_log_fmt("[-] ELF is not compatible with x86_64! header.isa = 0x%x\n", header.header.isa);
        return -1;
    } else if (header.header.type != ELF_TYPE_EXEC) {
        debug_log("[-] ELF is not an executable\n");
        return -1;
    }
    debug_log_fmt("[*] Found %d program headers\n", header.pht_entry_count);

    elf64_psh_t psh;

    for (int i = 0; i < header.pht_entry_count; i++) {
        if (file_seek(file, header.pht_offset + i * header.pht_entry_size, SEEK_SET) < 0)
            return -1;

        if (parse_elf_program_header(file, &psh, header.pht_entry_size) < 0)
            return -1;

        uintptr_t vaddr = psh.section_vaddr;
        uintptr_t vaddr_end = vaddr + psh.section_memory_size;
        uintptr_t vaddr_start = vaddr & ~(PAGE_SIZE - 1);
        uintptr_t vaddr_end_aligned = PAGE_UP(vaddr_end);
        size_t npages = (vaddr_end_aligned - vaddr_start) / PAGE_SIZE;

        void *phys_mem = pmm_alloc(npages);
        if (!phys_mem) {
            debug_log("[-] Failed to allocate physical memory for segment\n");
            return -1;
        }

        uint64_t flags = PTFLAG_US | PTFLAG_P | PTFLAG_RW | PTFLAG_XD;
        vmm_map_range(vaddr_start, (uintptr_t) phys_mem, npages * PAGE_SIZE, flags, false);

        void *hddm_addr = vmm_get_hhdm_addr(phys_mem);
        memset(hddm_addr, 0, npages * PAGE_SIZE);

        size_t offset_in_page = vaddr - vaddr_start;
        if (file_seek(file, psh.section_offset, SEEK_SET) < 0) {
            debug_log("[-] Failed to seek to segment offset\n");
            pmm_free(phys_mem, npages);
            return -1;
        }

        if (file_read(file, hddm_addr + offset_in_page, psh.section_file_size) < 0) {
            debug_log("[-] Failed to read segment data\n");
            pmm_free(phys_mem, npages);
            return -1;
        }
    }

    void *stack = pmm_alloc(10);
    vmm_map_range(
        0x00007fffe0000000ULL,
        (uintptr_t) stack,
        10 * PAGE_SIZE,
        PTFLAG_US | PTFLAG_RW | PTFLAG_P,
        false);

    void *kernelstack = pmm_alloc(10);
    vmm_map_range(
        -(10LL * PAGE_SIZE),
        (uintptr_t) kernelstack,
        10 * PAGE_SIZE,
        PTFLAG_US | PTFLAG_RW | PTFLAG_P,
        false);

    asm_write_cr3(asm_read_cr3());
    uintptr_t stack_top = 0x00007fffe0000000ULL + 10 * PAGE_SIZE;
    jump_usermode((func_t) header.entry_offset, (void *) stack_top);

    return 0;
}
