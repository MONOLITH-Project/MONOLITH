/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "kernel/serial.h"
#include <kernel/arch/pc/gdt.h>

/*
 * Global Descriptor Table Entry Descriptor.
 * https://wiki.osdev.org/Global_Descriptor_Table#Segment_Descriptor
 */
typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) tss_entry_t;

/*
 * Global Descriptor Table.
 * https://wiki.osdev.org/Global_Descriptor_Table#Table
 */
struct
{
    gdt_entry_t entries[5];
    tss_entry_t tss;
} __attribute__((packed)) gdt;

/*
 * GDTR structure.
 * https://wiki.osdev.org/Global_Descriptor_Table#GDTR
 */
struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdtr = {0};

/*
 * Task State Segment structure.
 * https://wiki.osdev.org/Task_State_Segment#Long_Mode
 */
static struct
{
    uint32_t prev_tss;
    uint32_t esp0; // Kernel stack pointer
    uint32_t ss0;  // Kernel stack segment
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) _tss = {0};

void init_gdt()
{
    debug_log("[*] Initializing the GDT...\n");

    set_gdt_gate(0, 0, 0, 0, 0);                // Null segment
    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    // TSS
    _tss.iomap_base = sizeof(_tss);
    load_tss(&_tss);

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint32_t) &gdt;

    debug_log("[*] Flushing the GDT...\n");
    flush_gdt();
    debug_log("[*] Flushing the TSS...\n");
    flush_tss();
    debug_log("[+] GDT initialized\n");
}

void set_gdt_gate(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt.entries[index].base_low = (base & 0xFFFF);
    gdt.entries[index].base_middle = (base >> 16) & 0xFF;
    gdt.entries[index].base_high = (base >> 24) & 0xFF;

    gdt.entries[index].limit_low = (limit & 0xFFFF);
    gdt.entries[index].granularity = (limit >> 16) & 0x0F;

    gdt.entries[index].granularity |= gran & 0xF0;
    gdt.entries[index].access = access;
}

void load_tss(void *tss)
{
    const uint32_t addr = (uint32_t) tss;
    gdt.tss.limit_low = sizeof(tss);
    gdt.tss.base_low = addr & 0xFFFF;
    gdt.tss.base_mid = (addr >> 16) & 0xFF;
    gdt.tss.access = 0x89;      // Present, DPL 0, 32-bit TSS
    gdt.tss.granularity = 0x00; // Limit high (0), G=0
    gdt.tss.base_high = (addr >> 24) & 0xFF;
}
