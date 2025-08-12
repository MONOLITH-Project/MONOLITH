/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/idt.h>
#include <kernel/debug.h>
#include <kernel/syscall/syscall.h>
#include <kernel/terminal/terminal.h>

#define GDT_KERNEL_CODE 1
#define GDT_KERNEL_DATA 2
#define GDT_USER_DATA 3
#define GDT_USER_CODE 4
#define GDT_RING_3 3
#define STAR_KCODE_OFFSET 32
#define STAR_UCODE_OFFSET 48
#define EFER_ENABLE_SYSCALL 1

extern void _syscall_handler();

void syscalls_init()
{
    idt_set_gate(0x80, _syscall_handler, IDT_TYPE_SOFTWARE);
}

void sys_hello()
{
    kprintf("\nSyscalls are working!\n");
    debug_log_fmt("Syscalls are working!\n");
}
