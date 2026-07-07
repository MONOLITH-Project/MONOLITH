/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/tasking/scheduler.h>
#include <kernel/tasking/task.h>
#include <stdbool.h>

static bool _initialized = false;

void scheduler_init()
{
    _initialized = true;
}

void scheduler_tick(void)
{
    if (!_initialized)
        return;

    task_t *current = task_get_current();
    if (current != NULL && current->quantum_remaining > 0)
        current->quantum_remaining--;
}

task_t *sched_next(void)
{
    task_t *current = task_get_current();
    if (!_initialized || current == NULL)
        return current;
    if (current->quantum_remaining > 0)
        return current;

    task_t *next = task_next(current);
    if (next != NULL)
        next->quantum_remaining = next->quantum;
    return next;
}
