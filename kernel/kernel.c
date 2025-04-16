/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/gdt.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/arch/pc/sse.h>
#include <kernel/klibc/io.h>
#include <kernel/memory/pmm.h>
#include <kernel/multiboot2.h>
#include <kernel/serial.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <kernel/video/vga.h>
#include <kernel/video/vga_terminal.h>

void kmain(struct multiboot_tag *multiboot_info)
{
    kshell_init();
    start_debug_serial(SERIAL_COM1);

    debug_log("[*] Searching for multiboot mmap tag...\n");
    struct multiboot_tag_mmap *mmap_info = find_mmap_tag(multiboot_info);
    if (mmap_info == NULL) {
        debug_log("[-] Could not find the memory map tag\n");
        while (1)
            __asm__("hlt");
    }
    pmm_init(mmap_info);

    init_sse();
    init_gdt();
    init_idt();

    terminal_t vga_terminal;
    vga_init_terminal(&vga_terminal);
    kshell_launch(&vga_terminal);

    while (1)
        __asm__("hlt");
}
