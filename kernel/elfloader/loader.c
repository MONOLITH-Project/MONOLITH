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

    elf64_psh_t *phs = kmalloc(header.pht_entry_count * header.pht_entry_size);
    if (!phs) {
        debug_log("[-] Failed to allocate memory for program headers\n");
        return -1;
    }

    size_t pages = 0;
    for (int i = 0; i < header.pht_entry_count; i++) {
        if (file_seek(file, header.pht_offset + i * header.pht_entry_size, SEEK_SET) < 0) {
            kfree(phs);
            return -1;
        }

        if (parse_elf_program_header(file, &phs[i], header.pht_entry_size) < 0) {
            kfree(phs);
            return -1;
        }

        debug_log_fmt("[*] Checking program header %d:\n", i);
        debug_log_fmt(
            "[*] Mapping program header 0x%x->0x%x\n", phs[i].section_paddr, phs[i].section_vaddr);
        debug_log_fmt("[*] Section is located at 0x%x\n", phs[i].section_offset);
        debug_log_fmt(
            "[*] Section size: %d bytes (%d pages)\n",
            phs[i].section_file_size,
            PAGE_UP(phs[i].section_file_size) / PAGE_SIZE);

        pages += PAGE_UP(phs[i].section_file_size) / PAGE_SIZE;
    }

    void *address_space = pmm_alloc(pages);
    if (!address_space) {
        debug_log("[-] Failed to allocate memory for address space\n");
        kfree(phs);
        return -1;
    }

    uintptr_t current_addr = ((uintptr_t) address_space);
    for (int i = 0; i < header.pht_entry_count; i++) {
        if (file_seek(file, phs[i].section_offset, SEEK_SET) < 0) {
            kfree(phs);
            pmm_free(address_space, pages);
            return -1;
        }

        void *hddm_addr = vmm_get_hhdm_addr((void *) current_addr);
        if (file_read(file, hddm_addr, phs[i].section_file_size) < 0) {
            kfree(phs);
            pmm_free(address_space, pages);
            return -1;
        }

        uint64_t flags = PTFLAG_US | PTFLAG_P;
        if (!(phs[i].flags & SECTION_FLAG_EXEC))
            flags |= PTFLAG_XD;
        if (phs[i].flags & SECTION_FLAG_WRITE)
            flags |= PTFLAG_RW;
        vmm_map(phs[i].section_vaddr, current_addr, flags, true);
        current_addr += PAGE_UP(phs[i].section_file_size);
    }

    void *stack = pmm_alloc(10);
    vmm_map_range(0x00007ffffffff000LL, (uintptr_t) stack, 10, 0b111, true);
    kfree(phs);

    void *kernelstack = pmm_alloc(10);
    vmm_map_range(-(10LL * PAGE_SIZE), (uintptr_t) kernelstack, 10 * PAGE_SIZE, 0b111, true);

    asm_write_cr3(asm_read_cr3());
    uintptr_t stack_top = 0x00007ffffffff000LL + 10 * PAGE_SIZE;
    jump_usermode((func_t) header.entry_offset, (void *) stack_top);

    pmm_free(address_space, pages);
    for (int i = 0; i < header.pht_entry_count; i++)
        vmm_unmap(phs[i].section_vaddr, true);
    vmm_unmap_range(-(10LL * PAGE_SIZE), 10 * PAGE_SIZE, true);

    return 0;
}
