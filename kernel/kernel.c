/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/gdt.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/arch/pc/sse.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/pmm.h>
#include <kernel/multiboot2.h>
#include <kernel/serial.h>
#include <kernel/video/console.h>

void kmain(struct multiboot_tag *multiboot_info)
{
    start_debug_serial(SERIAL_COM1);

    pmm_init(multiboot_info);
    heap_init(10);
    sse_init();
    gdt_init();
    idt_init();
    console_init(multiboot_info);

    while (1)
        __asm__("hlt");
}
