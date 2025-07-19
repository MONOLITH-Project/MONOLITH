/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/gdt.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/arch/pc/sse.h>
#include <kernel/fs/tmpfs.h>
#include <kernel/fs/vfs.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/serial.h>
#include <kernel/timer.h>
#include <kernel/video/console.h>
#include <libs/limine/limine.h>

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request
    = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests"))) volatile struct limine_memmap_request
    limine_mmap_request
    = {.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

void kmain()
{
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        while (1)
            __asm__("hlt");
    }

    start_debug_serial(SERIAL_COM1);

    sse_init();
    gdt_init();
    idt_init();
    pmm_init(limine_mmap_request.response);
    vmm_init(limine_mmap_request.response);
    heap_init(10);
    timer_init();
    tmpfs_new_drive();
    console_init(framebuffer_request.response);

    while (1)
        __asm__("hlt");
}
