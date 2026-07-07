/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/apic.h>
#include <kernel/arch/pc/asm.h>
#include <kernel/arch/pc/ia32/smp.h>
#include <kernel/arch/pc/interrupts.h>
#include <kernel/arch/pc/pic.h>
#include <stddef.h>
#include <stdint.h>

void interrupts_eoi(uint8_t isr)
{
    if (pic_is_enabled())
        pic_eoi(isr);
    else if (apic_is_initialized())
        apic_eoi();
}

void interrupts_set_irq_mask(uint8_t irq, bool mask)
{
    if (pic_is_enabled())
        pic_set_irq_mask(irq, mask);
    else if (apic_is_initialized())
        apic_set_irq_mask(irq, mask);
}

static uint64_t _interrupt_disable_count[SMP_MAX_CPUS];
static bool _first_interrupt_enable[SMP_MAX_CPUS] = {true};

static size_t _interrupt_cpu_index(void)
{
    size_t cpu = smp_current_cpu_index();
    return cpu < SMP_MAX_CPUS ? cpu : 0;
}

void interrupts_disable()
{
    size_t cpu = _interrupt_cpu_index();
    if (_interrupt_disable_count[cpu] == 0)
        asm_cli();
    _interrupt_disable_count[cpu]++;
}

void interrupts_enable()
{
    size_t cpu = _interrupt_cpu_index();
    if (_interrupt_disable_count[cpu] == 0) {
        if (_first_interrupt_enable[cpu]) {
            asm_sti();
            _first_interrupt_enable[cpu] = false;
        }
        return;
    }
    _interrupt_disable_count[cpu]--;
    if (_interrupt_disable_count[cpu] == 0)
        asm_sti();
}
