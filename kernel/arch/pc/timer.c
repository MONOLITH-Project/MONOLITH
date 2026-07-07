/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/apic.h>
#include <kernel/arch/pc/asm.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/arch/pc/interrupts.h>
#include <kernel/arch/pc/pit.h>
#if defined(__x86_64__)
#include <kernel/arch/pc/x64/smp.h>
#elif defined(__i386__)
#include <kernel/arch/pc/ia32/smp.h>
#endif
#include <kernel/devices/debug.h>
#include <kernel/spinlock.h>
#include <kernel/tasking/scheduler.h>
#include <kernel/tasking/task.h>
#include <kernel/timer.h>
#include <stdbool.h>
#include <stdint.h>

#define TIMER_CALIBRATION_TIMEOUT 10000000

typedef struct timer_block timer_block_t;
struct timer_block
{
    struct timer_block *next;
    volatile uint64_t countdown;
    task_t *task;
};

static timer_block_t *_base;
static volatile uint64_t _tick_count = 0;
static spinlock_t _timer_lock = SPINLOCK_INIT;
#if defined(__x86_64__) || defined(__i386__)
static uint32_t _lapic_timer_initial_count = 0;
#endif

static interrupt_registers_t *_timer_schedule(interrupt_registers_t *regs)
{
    scheduler_tick();
    task_t *next = sched_next();
    if (next != NULL && next != task_get_current())
        return task_switch_from_interrupt(regs, next);
    return regs;
}

static interrupt_registers_t *_timer_irq(interrupt_registers_t *regs)
{
#if defined(__x86_64__) || defined(__i386__)
    if (smp_current_cpu_index() != 0) {
        return _timer_schedule(regs);
    }
#endif

    _tick_count++;

    spinlock_lock(&_timer_lock);
    timer_block_t *current = _base;
    timer_block_t *prev = NULL;

    while (current != NULL) {
        if (current->countdown > 0)
            current->countdown--;

        if (current->countdown == 0) {
            timer_block_t *next = current->next;
            if (current->task != NULL && current->task->state == TASK_STATE_SLEEPING)
                task_set_state(current->task, TASK_STATE_RUNNABLE);
            if (prev == NULL) {
                _base = next;
            } else {
                prev->next = next;
            }
            current = next;
        } else {
            prev = current;
            current = current->next;
        }
    }
    spinlock_unlock(&_timer_lock);

#if defined(__x86_64__) || defined(__i386__)
    return _timer_schedule(regs);
#else
    return _timer_schedule(regs);
#endif
}

void timer_init()
{
    debug_log("Initializing the timer\n");
    pit_set_frequency(1000);
    irq_register_handler(0, _timer_irq);
    debug_log("Initialized the timer\n");
    interrupts_enable();
}

void timer_init_cpu()
{
#if defined(__x86_64__) || defined(__i386__)
    if (!apic_is_initialized())
        return;

    if (smp_current_cpu_index() == 0) {
        if (_lapic_timer_initial_count != 0)
            return;

        uint64_t start = _tick_count;
        size_t spin = 0;
        while (_tick_count == start && spin++ < TIMER_CALIBRATION_TIMEOUT)
            asm_pause();
        if (_tick_count == start)
            return;

        start = _tick_count;
        apic_timer_prepare_calibration();
        spin = 0;
        while ((_tick_count - start) < 10 && spin++ < TIMER_CALIBRATION_TIMEOUT)
            asm_pause();
        if ((_tick_count - start) < 10) {
            apic_timer_mask();
            return;
        }

        uint32_t elapsed = UINT32_MAX - apic_timer_current_count();
        apic_timer_mask();
        _lapic_timer_initial_count = elapsed / 10;
        if (_lapic_timer_initial_count == 0)
            _lapic_timer_initial_count = 1;
        return;
    }

    if (_lapic_timer_initial_count != 0)
        apic_timer_start_periodic(32, _lapic_timer_initial_count);
#endif
}

void sleep(uint64_t ms)
{
    timer_block_t block;
    block.countdown = ms;
    block.task = NULL;

    /* Mark task as sleeping */
    interrupts_disable();

    task_t *current = task_get_current();
    if (current) {
        task_set_state(current, TASK_STATE_SLEEPING);
        current->quantum = 0;
        current->quantum_remaining = 0;
        block.task = current;
    }

    spinlock_lock(&_timer_lock);
    block.next = _base;
    _base = &block;
    spinlock_unlock(&_timer_lock);

    interrupts_enable();

    if (current) {
        task_t *next = task_next(current);
        if (!next || next == current)
            next = task_idle();
        task_switch(next);
    }

    if (current) {
        task_set_state(current, TASK_STATE_RUNNABLE);
        current->quantum = DEFAULT_QUANTUM;
    }

    while (block.countdown > 0)
        asm_pause();
}

uint64_t timer_get_ticks()
{
    return _tick_count;
}
