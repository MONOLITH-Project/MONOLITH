/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/apic.h>
#include <kernel/arch/pc/asm.h>
#include <kernel/arch/pc/gdt.h>
#include <kernel/arch/pc/ia32/smp.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/arch/pc/sse.h>
#include <kernel/devices/debug.h>
#include <kernel/klibc/memory.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/vmm.h>
#include <kernel/tasking/syscall.h>
#include <kernel/tasking/task.h>
#include <kernel/timer.h>
#include <kernel/types.h>

#define SMP_AP_STACK_SIZE 0x4000
#define SMP_TRAMPOLINE_ADDR 0x7000
#define SMP_TRAMPOLINE_VECTOR (SMP_TRAMPOLINE_ADDR >> 12)
#define SMP_STARTUP_TIMEOUT 10000000
#define SMP_IPI_DELAY 100000

typedef struct
{
    uint32_t lapic_id;
    bool bsp;
    volatile bool online;
    uintptr_t stack_top;
    uintptr_t cr3;
} smp_cpu_t;

static smp_cpu_t _cpus[SMP_MAX_CPUS];
static size_t _cpu_count = 1;
static volatile size_t _online_cpu_count = 1;
static volatile bool _scheduler_started = false;

extern uint8_t smp_trampoline_start[];
extern uint8_t smp_trampoline_end[];
extern uint32_t smp_trampoline_cr3;
extern uint32_t smp_trampoline_stack_top;
extern uint32_t smp_trampoline_cpu;
extern uint32_t smp_trampoline_entry;

void smp_ap_main(smp_cpu_t *cpu);

static void _delay(void)
{
    for (size_t i = 0; i < SMP_IPI_DELAY; i++)
        asm_pause();
}

static void _patch_trampoline(smp_cpu_t *cpu)
{
    uint8_t *dst = (uint8_t *) SMP_TRAMPOLINE_ADDR;
    size_t size = (size_t) (smp_trampoline_end - smp_trampoline_start);
    memcpy(dst, smp_trampoline_start, size);

    size_t cr3_off = (uint8_t *) &smp_trampoline_cr3 - smp_trampoline_start;
    size_t stack_off = (uint8_t *) &smp_trampoline_stack_top - smp_trampoline_start;
    size_t cpu_off = (uint8_t *) &smp_trampoline_cpu - smp_trampoline_start;
    size_t entry_off = (uint8_t *) &smp_trampoline_entry - smp_trampoline_start;

    *(uint32_t *) (dst + cr3_off) = (uint32_t) cpu->cr3;
    *(uint32_t *) (dst + stack_off) = (uint32_t) cpu->stack_top;
    *(uint32_t *) (dst + cpu_off) = (uint32_t) (uintptr_t) cpu;
    *(uint32_t *) (dst + entry_off) = (uint32_t) (uintptr_t) smp_ap_main;
}

void smp_ap_main(smp_cpu_t *cpu)
{
    sse_init();
    idt_flush();
    apic_init_local();
    gdt_reload_tss();
    syscalls_init();

    __atomic_store_n(&cpu->online, true, __ATOMIC_RELEASE);
    __atomic_add_fetch(&_online_cpu_count, 1, __ATOMIC_RELEASE);

    while (!__atomic_load_n(&_scheduler_started, __ATOMIC_ACQUIRE))
        asm_pause();

    task_switching_init_cpu();
    timer_init_cpu();
    while (1) {
        if (task_current_cpu_has_runnable())
            task_switch(NULL);
        asm_pause();
    }
}

void smp_init(void)
{
    if (!apic_is_initialized()) {
        debug_log("SMP unavailable; APIC is not initialized\n");
        return;
    }

    size_t apic_cpu_count = apic_get_processor_count();
    if (apic_cpu_count == 0) {
        debug_log("SMP unavailable; no MADT processor entries\n");
        return;
    }

    _cpu_count = apic_cpu_count > SMP_MAX_CPUS ? SMP_MAX_CPUS : apic_cpu_count;
    uint32_t bsp_lapic_id = apic_get_lapic_id();
    size_t bsp_index = 0;

    for (size_t i = 0; i < _cpu_count; i++) {
        uint32_t lapic_id = apic_get_processor_lapic_id(i);
        bool bsp = lapic_id == bsp_lapic_id;
        _cpus[i].lapic_id = lapic_id;
        _cpus[i].bsp = bsp;
        _cpus[i].online = bsp;
        if (bsp)
            bsp_index = i;
    }

    _online_cpu_count = 1;
    for (size_t i = 0; i < _cpu_count; i++) {
        if (i == bsp_index)
            continue;

        void *stack = kmalloc(SMP_AP_STACK_SIZE);
        if (stack == NULL) {
            debug_log_fmt("Failed to allocate stack for CPU APIC ID %d\n", (int) _cpus[i].lapic_id);
            continue;
        }

        _cpus[i].stack_top = (uintptr_t) stack + SMP_AP_STACK_SIZE;
        _cpus[i].cr3 = vmm_get_kernel_cr3();
        _patch_trampoline(&_cpus[i]);

        apic_send_init_ipi(_cpus[i].lapic_id);
        _delay();
        apic_send_startup_ipi(_cpus[i].lapic_id, SMP_TRAMPOLINE_VECTOR);
        _delay();
        apic_send_startup_ipi(_cpus[i].lapic_id, SMP_TRAMPOLINE_VECTOR);

        size_t spin = 0;
        while (!__atomic_load_n(&_cpus[i].online, __ATOMIC_ACQUIRE)
               && spin++ < SMP_STARTUP_TIMEOUT)
            asm_pause();

        if (!_cpus[i].online)
            debug_log_fmt("CPU APIC ID %d failed to start\n", (int) _cpus[i].lapic_id);
    }

    debug_log_fmt("SMP initialized: %d/%d CPUs online\n", _online_cpu_count, _cpu_count);
}

void smp_start_scheduler(void)
{
    __atomic_store_n(&_scheduler_started, true, __ATOMIC_RELEASE);
}

size_t smp_current_cpu_index(void)
{
    uint32_t lapic_id = apic_get_lapic_id();
    for (size_t i = 0; i < _cpu_count; i++) {
        if (_cpus[i].lapic_id == lapic_id)
            return i;
    }
    return 0;
}

size_t smp_online_cpu_count(void)
{
    return __atomic_load_n(&_online_cpu_count, __ATOMIC_ACQUIRE);
}
