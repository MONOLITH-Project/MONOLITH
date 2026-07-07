/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/gdt.h>
#include <kernel/arch/pc/ia32/smp.h>
#include <kernel/devices/debug.h>

gdt_t gdt = {0};
gdtr_t gdtr = {0};

static gdt_t _cpu_gdts[SMP_MAX_CPUS];
static gdtr_t _cpu_gdtrs[SMP_MAX_CPUS];
static tss_entry_t _tss[SMP_MAX_CPUS];
extern uintptr_t syscall_kernel_stack_top;
extern void gdt_flush_with(gdtr_t *gdtr);

static size_t _current_cpu_index(void)
{
    size_t cpu = smp_current_cpu_index();
    return cpu < SMP_MAX_CPUS ? cpu : 0;
}

static void _gdt_set_gate(
    gdt_t *table, int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    table->entries[index].base_low = base & 0xFFFF;
    table->entries[index].base_middle = (base >> 16) & 0xFF;
    table->entries[index].base_high = (base >> 24) & 0xFF;
    table->entries[index].limit_low = limit & 0xFFFF;
    table->entries[index].granularity = (limit >> 16) & 0x0F;
    table->entries[index].granularity |= gran & 0xF0;
    table->entries[index].access = access;
}

static void _gdt_tss_load(gdt_t *table, void *tss)
{
    uint32_t base = (uint32_t) (uintptr_t) tss;
    uint32_t limit = sizeof(tss_entry_t) - 1;
    _gdt_set_gate(table, 5, base, limit, 0x89, 0x40);
}

static void _gdt_init_table(size_t cpu)
{
    gdt_t *table = &_cpu_gdts[cpu];

    _gdt_set_gate(table, 0, 0, 0, 0, 0);                /* Null segment */
    _gdt_set_gate(table, 1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* Code segment */
    _gdt_set_gate(table, 2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* Data segment */
    _gdt_set_gate(table, 3, 0, 0xFFFFFFFF, 0xFA, 0xCF); /* User mode code segment */
    _gdt_set_gate(table, 4, 0, 0xFFFFFFFF, 0xF2, 0xCF); /* User mode data segment */

    _tss[cpu].ss0 = 0x10;
    _tss[cpu].iomap_base = sizeof(_tss[cpu]);
    _gdt_tss_load(table, &_tss[cpu]);

    _cpu_gdtrs[cpu].limit = sizeof(*table) - 1;
    _cpu_gdtrs[cpu].base = (uint32_t) (uintptr_t) table;
}

void gdt_init()
{
    debug_log("Initializing the GDT...\n");

    _tss[0].esp0 = 0x9FF00;
    syscall_kernel_stack_top = _tss[0].esp0;
    _gdt_init_table(0);
    gdt = _cpu_gdts[0];
    gdtr = _cpu_gdtrs[0];

    debug_log("Flushing the GDT...\n");
    gdt_flush_with(&_cpu_gdtrs[0]);
    debug_log("Flushing the TSS...\n");
    gdt_flush_tss();
    debug_log("Initialized the GDT\n");
}

void gdt_tss_set_rsp0(uint64_t rsp0)
{
    size_t cpu = _current_cpu_index();
    _tss[cpu].esp0 = (uint32_t) rsp0;
    _tss[cpu].ss0 = 0x10;
    _tss[cpu].iomap_base = sizeof(_tss[cpu]);
    syscall_kernel_stack_top = (uintptr_t) rsp0;
}

void gdt_reload_tss(void)
{
    size_t cpu = _current_cpu_index();
    _gdt_init_table(cpu);
    gdt_flush_with(&_cpu_gdtrs[cpu]);
    gdt_flush_tss();
}

void gdt_set_gate(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    _gdt_set_gate(&gdt, index, base, limit, access, gran);
}

void gdt_tss_load(void *tss)
{
    _gdt_tss_load(&gdt, tss);
}
