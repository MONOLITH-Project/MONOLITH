/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "gdt/gdt.h"
#include "idt/idt.h"
#include "klibc/io.h"
#include "memory/pmm.h"
#include "multiboot2.h"
#include "serial.h"
#include "sse/sse.h"

void print_hello_world()
{
    const char *str = "Hello, World!";
    char *vidptr = (char *) 0xb8000;
    unsigned int i = 0;
    unsigned int j = 0;

    while (str[j] != '\0') {
        vidptr[i] = str[j];
        vidptr[i + 1] = 0x07;
        ++j;
        i += 2;
        outb(0x0E, 0x3D4);
        outb((j >> 8) & 0xFF, 0x3D5);
        outb(0x0F, 0x3D4);
        outb(j & 0xFF, 0x3D5);
    }
}

void kmain(struct multiboot_tag *multiboot_info)
{
    start_debug_serial(SERIAL_COM1);

    debug_log("[*] Searching for multiboot mmap tag...\n");
    struct multiboot_tag_mmap *mmap_info = find_mmap_tag(multiboot_info);
    if (mmap_info == NULL) {
        debug_log("[-] Could not find the memory map tag\n");
        while (1)
            __asm__("hlt");
    }
    init_pmm(mmap_info);

    init_sse();
    init_gdt();
    init_idt();

    print_hello_world();

    while (1)
        __asm__("hlt");
}
