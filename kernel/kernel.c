/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/gdt.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/arch/pc/sse.h>
#include <kernel/klibc/io.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/memstats.h>
#include <kernel/memory/pmm.h>
#include <kernel/multiboot2.h>
#include <kernel/serial.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <kernel/video/vga.h>
#include <kernel/video/vga_terminal.h>

void kmain(struct multiboot_tag *multiboot_info)
{
    start_debug_serial(SERIAL_COM1);

    pmm_init(multiboot_info);
    heap_init(10);
    sse_init();
    gdt_init();
    idt_init();

    terminal_t vga_terminal;
    vga_init_terminal(&vga_terminal);
    kshell_init();
    memstats_init_cmds();

    kshell_launch(&vga_terminal);

    while (1)
        __asm__("hlt");
}
