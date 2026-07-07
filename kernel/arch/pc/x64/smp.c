/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/apic.h>
#include <kernel/arch/pc/asm.h>
#include <kernel/arch/pc/gdt.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/arch/pc/sse.h>
#include <kernel/arch/pc/x64/smp.h>
#include <kernel/devices/debug.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/vmm.h>
#include <kernel/tasking/syscall.h>
#include <kernel/tasking/task.h>
#include <kernel/timer.h>
#include <kernel/types.h>

#define SMP_AP_STACK_SIZE 0x4000
#define SMP_STARTUP_TIMEOUT 10000000

typedef struct
{
    uint32_t processor_id;
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

extern void smp_ap_entry(struct limine_mp_info *mp_info);

void smp_ap_main(struct limine_mp_info *mp_info)
{
    smp_cpu_t *cpu = (smp_cpu_t *) mp_info->extra_argument;

    sse_init();
    idt_flush();
    gdt_reload_tss();
    syscalls_init();
    apic_init_local();

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

void smp_init(struct limine_mp_response *mp_response)
{
    if (mp_response == NULL || mp_response->cpu_count == 0) {
        debug_log("SMP unavailable; booting with one CPU\n");
        return;
    }

    size_t limine_cpu_count = mp_response->cpu_count;
    _cpu_count = limine_cpu_count > SMP_MAX_CPUS ? SMP_MAX_CPUS : limine_cpu_count;

    size_t bsp_index = 0;
    for (size_t i = 0; i < _cpu_count; i++) {
        struct limine_mp_info *mp_info = mp_response->cpus[i];
        bool bsp = mp_info->lapic_id == mp_response->bsp_lapic_id;
        _cpus[i].processor_id = mp_info->processor_id;
        _cpus[i].lapic_id = mp_info->lapic_id;
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
            debug_log_fmt("Failed to allocate stack for CPU %d\n", (int) _cpus[i].processor_id);
            continue;
        }

        _cpus[i].stack_top = (uintptr_t) stack + SMP_AP_STACK_SIZE;
        _cpus[i].cr3 = vmm_get_kernel_cr3();
        mp_response->cpus[i]->extra_argument = (uint64_t) &_cpus[i];
        __atomic_store_n(&_cpus[i].online, false, __ATOMIC_RELEASE);
        __atomic_store_n(&mp_response->cpus[i]->goto_address, smp_ap_entry, __ATOMIC_RELEASE);
    }

    for (size_t i = 0; i < _cpu_count; i++) {
        if (i == bsp_index)
            continue;

        for (size_t spin = 0; spin < SMP_STARTUP_TIMEOUT; spin++) {
            if (__atomic_load_n(&_cpus[i].online, __ATOMIC_ACQUIRE))
                break;
            asm_pause();
        }

        if (!__atomic_load_n(&_cpus[i].online, __ATOMIC_ACQUIRE))
            debug_log_fmt("CPU %d did not come online\n", (int) _cpus[i].processor_id);
    }

    debug_log_fmt("SMP initialized: %d/%d CPUs online\n", smp_online_cpu_count(), _cpu_count);
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

size_t smp_online_cpu_count()
{
    return __atomic_load_n(&_online_cpu_count, __ATOMIC_ACQUIRE);
}
