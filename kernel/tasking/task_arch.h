/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <kernel/tasking/task.h>
#include <stddef.h>
#include <stdint.h>

struct interrupt_registers;

size_t task_arch_current_cpu(void);
size_t task_arch_online_cpu_count(void);
uintptr_t task_arch_user_space_start(void);
uintptr_t task_arch_user_space_end(void);

void task_arch_install_switch_gate(void);
void task_arch_init_idle_context(task_t *task);
void task_arch_init_task_context(task_t *task, void *entry_point, task_mode_t mode);
void task_arch_load_context(task_t *task);
void task_arch_switch_trap(void);

void task_arch_state_save(task_t *task, struct interrupt_registers *regs);
void task_arch_state_load(task_t *task, struct interrupt_registers *regs);
struct interrupt_registers *task_arch_frame_for_load(
    task_t *task, struct interrupt_registers *fallback);
struct interrupt_registers *task_switch_gate(struct interrupt_registers *regs);
