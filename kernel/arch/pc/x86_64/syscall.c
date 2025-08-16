/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "kernel/input/ps2_keyboard.h"
#include "kernel/input/ps2_mouse.h"
#include <kernel/arch/pc/idt.h>
#include <kernel/debug.h>
#include <kernel/klibc/memory.h>
#include <kernel/memory/vmm.h>
#include <kernel/syscall/syscall.h>
#include <kernel/terminal/terminal.h>
#include <stdint.h>

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

extern struct limine_framebuffer_request framebuffer_request;
int sys_request_fb(void *fb_info)
{
    if (framebuffer_request.response->framebuffer_count < 1)
        return -1;
    size_t width = framebuffer_request.response->framebuffers[0]->width;
    size_t height = framebuffer_request.response->framebuffers[0]->height;
    void *lhfb = vmm_get_lhdm_addr(framebuffer_request.response->framebuffers[0]->address);
    void *hhfb = framebuffer_request.response->framebuffers[0]->address;
    vmm_map_range((uintptr_t) hhfb, (uintptr_t) lhfb, width * height * 4, 0b111, true);
    memcpy(fb_info, &hhfb, sizeof(void *));
    memcpy(fb_info + sizeof(void *), &width, sizeof(uint64_t));
    memcpy(fb_info + 2 * sizeof(void *), &height, sizeof(uint64_t));
    return 0;
}

int sys_register_mouse_handler(ps2_mouse_event_handler_t handler)
{
    ps2_mouse_register_event_handler(handler);
    return 0;
}
