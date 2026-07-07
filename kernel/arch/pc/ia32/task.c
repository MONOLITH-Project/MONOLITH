/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/asm.h>
#include <kernel/arch/pc/gdt.h>
#include <kernel/arch/pc/ia32/smp.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/arch/pc/sse.h>
#include <kernel/klibc/memory.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/vmm.h>
#include <kernel/tasking/task_arch.h>
#include <stddef.h>

#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR 0x1B
#define USER_DATA_SELECTOR 0x23
#define DEFAULT_RFLAGS 0x202
#define USER_SPACE_START 0x10000000UL
#define USER_SPACE_END 0xBFF00000UL

extern void _task_switch_gate_stub();

static void _task_init_fx_state(task_t *task)
{
    if (task->regs.fx_state_aligned != NULL)
        return;

    task->regs.fx_state = kmalloc(512 + 16);
    if (task->regs.fx_state == NULL)
        return;

    uintptr_t fx_addr = (uintptr_t) task->regs.fx_state;
    task->regs.fx_state_aligned = (void *) ((fx_addr + 15) & ~((uintptr_t) 0xF));
    memset(task->regs.fx_state_aligned, 0, 512);

    uint8_t *fx_region = (uint8_t *) task->regs.fx_state_aligned;
    *((uint16_t *) &fx_region[0]) = 0x037F;
    *((uint32_t *) &fx_region[24]) = 0x1F80;
}

size_t task_arch_current_cpu(void)
{
    size_t cpu = smp_current_cpu_index();
    return cpu < SMP_MAX_CPUS ? cpu : 0;
}

size_t task_arch_online_cpu_count(void)
{
    return smp_online_cpu_count();
}

uintptr_t task_arch_user_space_start(void)
{
    return USER_SPACE_START;
}

uintptr_t task_arch_user_space_end(void)
{
    return USER_SPACE_END;
}

void task_arch_install_switch_gate(void)
{
    idt_set_gate(0x30, _task_switch_gate_stub, IDT_TYPE_INTERRUPT);
}

void task_arch_init_idle_context(task_t *task)
{
    task->regs.cr3 = vmm_get_kernel_cr3();
    task->regs.cs = KERNEL_CODE_SELECTOR;
    task->regs.ss = KERNEL_DATA_SELECTOR;
    task->regs.rflags = DEFAULT_RFLAGS;
    task->regs.rsp = asm_read_rsp();
    task->regs.rsp0 = task->regs.rsp;
    _task_init_fx_state(task);
}

void task_arch_init_task_context(task_t *task, void *entry_point, task_mode_t mode)
{
    task->regs.rip = (uintptr_t) entry_point;
    task->regs.rflags = DEFAULT_RFLAGS;
    task->regs.cs = mode == TASK_MODE_USER ? USER_CODE_SELECTOR : KERNEL_CODE_SELECTOR;
    task->regs.ss = mode == TASK_MODE_USER ? USER_DATA_SELECTOR : KERNEL_DATA_SELECTOR;

    if (mode == TASK_MODE_USER)
        return;

    interrupt_registers_t *frame
        = (interrupt_registers_t *) (task->regs.rsp0 - sizeof(interrupt_registers_t));
    memset(frame, 0, sizeof(*frame));
    frame->rip = task->regs.rip;
    frame->cs = KERNEL_CODE_SELECTOR;
    frame->rflags = task->regs.rflags;
    frame->rsp = (uintptr_t) &frame->rsp;
    frame->ss = KERNEL_DATA_SELECTOR;
    task->regs.rsp = frame->rsp;
    task->kernel_frame_valid = true;
    task->kernel_frame_has_stack_slots = false;
}

void task_arch_load_context(task_t *task)
{
    gdt_tss_set_rsp0(task->regs.rsp0);
    if (task->regs.cr3)
        asm_write_cr3(task->regs.cr3);
}

void task_arch_switch_trap(void)
{
    __asm__ volatile("int $0x30");
}

void task_arch_state_save(task_t *task, interrupt_registers_t *regs)
{
    task->regs.rax = regs->rax;
    task->regs.rbx = regs->rbx;
    task->regs.rcx = regs->rcx;
    task->regs.rdx = regs->rdx;
    task->regs.rsi = regs->rsi;
    task->regs.rdi = regs->rdi;
    task->regs.rbp = regs->rbp;
    task->regs.rip = regs->rip;
    task->regs.rsp = regs->rsp;
    task->regs.rflags = regs->rflags ? regs->rflags : DEFAULT_RFLAGS;
    task->regs.cr3 = asm_read_cr3();

    uint16_t cs = (uint16_t) regs->cs;
    uint16_t ss = (uint16_t) regs->ss;
    task->regs.cs = cs ? cs : (task->user_mode ? USER_CODE_SELECTOR : KERNEL_CODE_SELECTOR);
    if ((task->regs.cs & 0x03) == 0) {
        task->regs.rsp = (uintptr_t) &regs->rsp;
        task->regs.ss = KERNEL_DATA_SELECTOR;
        task->kernel_frame_valid = true;
        task->kernel_frame_has_stack_slots = false;
    } else {
        task->regs.rsp = regs->rsp;
        task->regs.ss = ss ? ss : USER_DATA_SELECTOR;
        task->kernel_frame_valid = false;
        task->kernel_frame_has_stack_slots = false;
    }

    if (task->regs.fx_state_aligned)
        sse_save(task->regs.fx_state_aligned);
}

struct interrupt_registers *task_arch_frame_for_load(
    task_t *task, struct interrupt_registers *fallback)
{
    if ((task->regs.cs & 0x03) == 0 && task->kernel_frame_valid)
        return (interrupt_registers_t *) (task->regs.rsp - offsetof(interrupt_registers_t, rsp));
    if (task->regs.rsp0 != 0)
        return (interrupt_registers_t *) (task->regs.rsp0 - sizeof(interrupt_registers_t));
    return fallback;
}

void task_arch_state_load(task_t *task, interrupt_registers_t *regs)
{
    regs->rax = task->regs.rax;
    regs->rbx = task->regs.rbx;
    regs->rcx = task->regs.rcx;
    regs->rdx = task->regs.rdx;
    regs->rsi = task->regs.rsi;
    regs->rdi = task->regs.rdi;
    regs->rbp = task->regs.rbp;
    regs->rip = task->regs.rip;
    regs->rflags = task->regs.rflags ? task->regs.rflags : DEFAULT_RFLAGS;

    uint16_t cs = task->regs.cs ? task->regs.cs
                                 : (task->user_mode ? USER_CODE_SELECTOR : KERNEL_CODE_SELECTOR);
    uint16_t ss = task->regs.ss ? task->regs.ss
                                 : (task->user_mode ? USER_DATA_SELECTOR : KERNEL_DATA_SELECTOR);
    if ((ss & 0x03) != (cs & 0x03))
        ss = (cs & 0x03) == 0 ? KERNEL_DATA_SELECTOR : USER_DATA_SELECTOR;

    regs->cs = cs;
    if ((cs & 0x03) != 0 || !task->kernel_frame_valid || task->kernel_frame_has_stack_slots) {
        regs->rsp = task->regs.rsp;
        regs->ss = ss;
    }

    if (task->regs.fx_state_aligned)
        sse_restore(task->regs.fx_state_aligned);
}

interrupt_registers_t *_task_switch_gate(interrupt_registers_t *regs)
{
    return (interrupt_registers_t *) task_switch_gate(regs);
}
