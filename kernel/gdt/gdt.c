/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "gdt.h"
#include "../serial.h"

/*
 * Global Descriptor Table Entry Descriptor.
 * https://wiki.osdev.org/Global_Descriptor_Table#Long_Mode_System_Segment_Descriptor
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
    uint16_t length;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t base_hi;
    uint32_t base_upper32;
    uint32_t reserved;
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
    uint64_t base;
} __attribute__((packed)) gdtr = {0};

/*
 * Task State Segment structure.
 * https://wiki.osdev.org/Task_State_Segment#Long_Mode
 */
static struct
{
    uint32_t reserved;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomap_base;
} __attribute__((packed)) _tss = {0};

void init_gdt()
{
    debug_log("[*] Initializing the GDT...\n");

    // https://wiki.osdev.org/GDT_Tutorial#Flat_/_Long_Mode_Setup
    set_gdt_gate(0, 0, 0, 0, 0);                // Null segment
    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0x20); // Code segment
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xA0); // Data segment
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xFA, 0x20); // User mode code segment
    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xA0); // User mode data segment

    // TSS
    _tss.iomap_base = sizeof(_tss);
    load_tss(&_tss);

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t) &gdt;

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
    const uint64_t addr = (uint64_t) tss;
    gdt.tss.length = sizeof(tss);
    gdt.tss.base_low = addr & 0xFFFF;
    gdt.tss.base_mid = (addr >> 16) & 0xFF;
    gdt.tss.base_hi = (addr >> 24) & 0xFF;
    gdt.tss.base_upper32 = addr >> 32;
    gdt.tss.flags1 = 0x89;
    gdt.tss.flags2 = 0x00;
    gdt.tss.reserved = 0;
}
