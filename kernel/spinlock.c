/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/asm.h>
#include <kernel/spinlock.h>
#include <stdint.h>

void spinlock_init(spinlock_t *lock)
{
    lock->locked = 0;
}

void spinlock_lock(spinlock_t *lock)
{
    while (__sync_lock_test_and_set(&lock->locked, 1))
        asm_pause();
}

void spinlock_unlock(spinlock_t *lock)
{
    __sync_lock_release(&lock->locked);
}

bool spinlock_lock_irqsave(spinlock_t *lock)
{
    uintptr_t rflags;
#if defined(__x86_64__)
    __asm__ volatile("pushfq; pop %0" : "=r"(rflags) :: "memory");
#else
    __asm__ volatile("pushfl; pop %0" : "=r"(rflags) :: "memory");
#endif
    asm_cli();
    spinlock_lock(lock);
    return (rflags & 0x200) != 0;
}

void spinlock_unlock_irqrestore(spinlock_t *lock, bool interrupts_enabled)
{
    spinlock_unlock(lock);
    if (interrupts_enabled)
        asm_sti();
}
