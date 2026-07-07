/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdbool.h>

typedef struct
{
    volatile int locked;
} spinlock_t;

#define SPINLOCK_INIT {0}

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);
bool spinlock_lock_irqsave(spinlock_t *lock);
void spinlock_unlock_irqrestore(spinlock_t *lock, bool interrupts_enabled);
